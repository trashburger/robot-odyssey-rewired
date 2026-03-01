import { Region2D } from 'region2d';

export type Rect =
    | { left: number; top: number; right: number; bottom: number }
    | [left: number, top: number, right: number, bottom: number]
    | { x: number; y: number; width: number; height: number };

// Build an optimized SVG path from a list of Rects.
export function pathFromRects(rects: Rect[]): string {
    let parts = [];
    const polygons = Region2D.fromRects(rects).getPath();
    for (let polygon of polygons) {
        let command = 'M';
        for (let point of polygon) {
            parts.push(command);
            parts.push(point.x.toString());
            parts.push(point.y.toString());
            command = 'L';
        }
        parts.push('Z');
    }
    return parts.join(' ');
}
