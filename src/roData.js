// Some information about Robot Odyssey world data,
// implemented directly in Javascript. See the main
// API for world data on the C++ side, in roData.h

const LAB_WORLD = 30;

export function worldIdFromSaveData(bytes)
{
    if (bytes.length === 24389) {
        return bytes[bytes.length - 1];
    }
    return null;
}

export function chipNameFromSaveData(bytes)
{
    if (bytes.length === 1333) {
        let name = '';
        const nameField = bytes.slice(0x40A, 0x41C);
        for (let byte of nameField) {
            if (byte < 0x20 || byte > 0x7F) {
                break;
            }
            name += String.fromCharCode(byte);
        }
        return name.trim() || 'Untitled Chip';
    }
    return null;
}

export function filenameForSaveData(bytes, date)
{
    const world = worldIdFromSaveData(bytes);
    const chip = chipNameFromSaveData(bytes);
    const datestr = date.toLocaleString().replace(/\//g,'-');

    if (world !== null) {
        if (world === LAB_WORLD) {
            return `Saved Lab (${datestr}).lsv`;
        } else {
            return `Robotropolis, World ${world+1} (${datestr}).gsv`;
        }
    }
    if (chip !==  null) {
        return `${chip} (${datestr}).csv`;
    }
    return `${datestr}.bin`;
}

export function filenameForAutosave(bytes, date)
{
    const world = worldIdFromSaveData(bytes);
    const datestr = date.toLocaleString().replace(/\//g,'-');

    if (world !== null) {
        if (world === LAB_WORLD) {
            return `Autosave (${datestr}).lsvz`;
        } else {
            return `Autosave (${datestr}).gsvz`;
        }
    }
    return `Autosave (${datestr}).bin`;
}