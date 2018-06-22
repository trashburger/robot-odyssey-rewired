export function init(engine)
{
    engine.settings = new Settings();
    engine.files = new Files();

    const indexedDB = window.indexedDB;
    if (!indexedDB) {
        return;
    }

    const request = indexedDB.open('robot-odyssey-rewired-storage', 1);

    request.onerror = (e) => {
        /* eslint-disable no-console */
        console.warn('Storage error, ' + e);
    };

    request.onupgradeneeded = (e) => {
        const db = e.target.result;

        db.createObjectStore('settings', { keyPath: 'setting' });

        const files = db.createObjectStore('files', { keyPath: 'name' });
        files.createIndex('extension', 'extension');
        files.createIndex('date', 'date');
    };

    request.onsuccess = (e) => {
        const db = e.target.result;
        engine.settings.init(db);
        engine.files.init(db);
    };
}


class Files
{
    constructor()
    {
        this.db = null;
        this.watchfn = {};
    }

    objectStore(mode)
    {
        return this.db.transaction(['files'], mode).objectStore('files');
    }

    init(db)
    {
        this.db = db;
    }

    save(name, data, date)
    {
        if (this.db) {
            date = date || new Date();
            const extension = name.split('.').pop();
            const file = { name, data, date, extension };
            this.objectStore('readwrite').put(file);
        }
    }
}


class Settings
{
    constructor()
    {
        // Even if the database never loads, callbacks need to work
        this.db = null;
        this.watchfn = {};
        this.values = {
            palette: { setting: 'palette', name: 'rewired' },
        };
    }

    objectStore(mode)
    {
        return this.db.transaction(['settings'], mode).objectStore('settings');
    }

    init(db)
    {
        this.db = db;

        // Retrieve initial values
        this.objectStore().openCursor().onsuccess = (e) => {
            const cursor = e.target.result;
            if (cursor) {
                this.values[cursor.key] = cursor.value;
                const fn = this.watchfn[cursor.key];
                if (fn) fn(cursor.value);
                cursor.continue();
            }
        };
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
        const fn = this.watchfn[key];
        if (fn) fn(obj);
        if (this.db) {
            this.objectStore('readwrite').put(obj);
        }
    }
}
