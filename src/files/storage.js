import idb from 'idb';
import { readAsArrayBuffer } from 'promise-file-reader';
import * as EngineLoader from '../engineLoader.js';

const MAX_FILESIZE = 0x10000;

export function init()
{
    const request = idb.open('robot-odyssey-rewired-storage', 1, upgradeDatabase);

    const engine = EngineLoader.instance;
    engine.settings = new Settings(request);
    engine.files = new Files(request, getStaticFiles());
}

export function isCompressed(file)
{
    return file.extension.includes('z');
}

function upgradeDatabase(db)
{
    db.createObjectStore('settings', { keyPath: 'setting' });
    const files = db.createObjectStore('files', { keyPath: 'name' });
    files.createIndex('extension', 'extension');
    files.createIndex('date', 'date');
}

function getExtension(name)
{
    return name.split('.').pop().toLowerCase();
}

async function getStaticFiles()
{
    const engine = await EngineLoader.complete;
    return engine.getStaticFiles();
}

function isHiddenFile(name)
{
    return /(^\.|[/\\]\.)/.test(name);
}

class Files
{
    constructor(dbPromise, staticFilesPromise)
    {
        this.dbPromise = dbPromise;
        this.staticFilesPromise = staticFilesPromise;
        this.maxFileSize = MAX_FILESIZE;
        this.allowedExtensions = 'gsv lsv csv gsvz lsvz'.split(' ');
    }

    async save(name, data, date)
    {
        date = date || new Date();
        data = new Uint8Array(data);
        const extension = getExtension(name);

        if (isHiddenFile(name)) {
            return null;
        }

        // Special file types: extract zip archives now
        if (extension === 'zip') {
            const JSZip = (await import('jszip')).default;
            const zip = await JSZip.loadAsync(data);
            return await this.saveZip(zip);
        }

        if (data.length > this.maxFileSize) {
            // Ignore files too big for the engine
            return null;
        }
        if (!this.allowedExtensions.includes(extension)) {
            // Ignore unsupported extensions
            return null;
        }

        const db = await this.dbPromise;
        const file = { name, data, date, extension };
        const tx = db.transaction('files', 'readwrite');
        const store = tx.objectStore('files');
        store.put(file);
        await tx.complete;

        return file;
    }

    async saveFile(file)
    {
        const date = new Date(file.lastModified);
        if (!file.name) {
            return null;
        }

        const data = await readAsArrayBuffer(file);
        return await this.save(file.name, data, date);
    }

    saveFiles(iter)
    {
        const promises = [];
        for (let f of iter) {
            promises.push(this.saveFile(f));
        }
        return Promise.all(promises);
    }

    async listFiles(fn)
    {
        // Put static files first, then wait on the database.
        // In the case of the chip loader (currently the only place where
        // this distinction matters) we want to present the built-in chips
        // quickly and consistently even if the db takes its time.

        const staticFiles = await this.staticFilesPromise;
        const date = new Date('1986-01-01T00:00:00.000Z');

        for (const [name, data] of Object.entries(staticFiles)) {
            const extension = getExtension(name);
            const file = { name, date, extension, data };
            file.load = () => Promise.resolve(file);
            fn(file);
        }

        const db = await this.dbPromise;
        const tx = db.transaction('files');

        tx.objectStore('files').index('date').iterateKeyCursor(null, 'prev', (cursor) => {
            if (!cursor) return;
            const name = cursor.primaryKey;
            const date = cursor.key;
            if (!isHiddenFile(name)) {
                const extension = getExtension(name);
                const file = { name, date, extension };
                file.load = () => this.loadFile(file);
                fn(file);
            }
            cursor.continue();
        });

        await tx.complete;
    }

    async loadFile(file)
    {
        const db = await this.dbPromise;
        const v = await db.transaction('files').objectStore('files').get(file.name);
        var data = v.data;

        // Object data must always be stored as a Uint8Array without a larger
        // than necessary backing buffer. But older versions didn't adhere to
        // this rule. Convert records as we encounter them.
        if (!(data instanceof Uint8Array) || data.buffer.byteLength !== data.length) {
            this.save(file.name, data, file.date);
            data = new Uint8Array(data);
        }

        // Cache file data after it's loaded once
        file.data = data;
        file.load = async () => file;
        return file;
    }

    async createZip()
    {
        const JSZip = (await import('jszip')).default;
        const zip = new JSZip();

        const db = await this.dbPromise;
        const tx = db.transaction('files');

        tx.objectStore('files').iterateCursor((cursor) => {
            if (!cursor) return;
            zip.file(cursor.value.name, cursor.value.data, {
                date: cursor.value.date,
                binary: true,
            });
            cursor.continue();
        });

        await tx.complete;
        return zip;
    }

    async createZipBlob()
    {
        const zip = await this.createZip();
        return await zip.generateAsync({
            type: 'blob'
        });
    }

    saveZip(zip)
    {
        const promises = [];
        zip.forEach((path, file) => {
            if (!file.dir) {
                promises.push(file.async('uint8array').then((data) => {
                    return this.save(file.name, data, file.date);
                }));
            }
        });
        return Promise.all(promises);
    }
}

class Settings
{
    constructor(dbPromise)
    {
        this.dbPromise = dbPromise;
        this.watchfn = {};
        this.values = {
            palette: { setting: 'palette', name: 'rewired' },
        };

        // Retrieve initial values
        this.init = this.dbPromise.then((db) => {
            const tx = db.transaction('settings');
            tx.objectStore('settings').iterateCursor((cursor) => {
                if (!cursor) return;
                this.values[cursor.key] = cursor.value;
                const fn = this.watchfn[cursor.key];
                if (fn) fn(cursor.value);
                cursor.continue();
            });
            return tx.complete;
        });
    }

    watch(key, fn)
    {
        // Append a watch function
        const prev = this.watchfn[key];
        this.watchfn[key] = (v) => {
            if (prev) prev(v);
            fn(v);
        };

        // Callback now if data is already available
        if (key in this.values) {
            fn(this.values[key]);
        }
    }

    put(obj)
    {
        const key = obj.setting;
        this.values[key] = obj;
        const fn = this.watchfn[key];
        if (fn) fn(obj);
        return this.dbPromise.then((db) => {
            const tx = db.transaction('settings', 'readwrite');
            const store = tx.objectStore('settings');
            store.put(obj);
            return tx.complete;
        });
    }
}
