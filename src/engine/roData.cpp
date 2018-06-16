#include <assert.h>
#include <string.h>
#include <vector>
#include "roData.h"


void ROWorld::clear()
{
    memset(this, 0, sizeof *this);
    memset(objects.nextInRoom, RO_OBJ_NONE, sizeof objects.nextInRoom);
    memset(objects.room, RO_ROOM_NONE, sizeof objects.room);
    memset(rooms.objectListHead, RO_OBJ_NONE, sizeof rooms.objectListHead);
    memset(text.room, RO_ROOM_NONE, sizeof text.room);
}

ROWorld *ROWorld::fromProcess(SBTProcess *proc)
{
    int addr = proc->getAddress(SBTADDR_WORLD_DATA);
    return addr < 0 ? 0 : reinterpret_cast<ROWorld*>(proc->memSeg(proc->reg.ds) + addr);
}

RORoomId ROWorld::getObjectRoom(ROObjectId obj)
{
    return (RORoomId) objects.room[obj];
}

void ROWorld::getObjectXY(ROObjectId obj, int &x, int &y)
{
    x = objects.x[obj];
    y = objects.y[obj];
}

void ROWorld::setObjectRoom(ROObjectId obj, RORoomId room)
{
    removeObjectFromRoom(obj, getObjectRoom(obj));
    objects.room[obj] = room;
    addObjectToRoom(obj, room);
}

void ROWorld::setObjectXY(ROObjectId obj, int x, int y)
{
    objects.x[obj] = x;
    objects.y[obj] = y;
}

void ROWorld::removeObjectFromRoom(ROObjectId obj, RORoomId room)
{
    /*
     * Remove an object from a room's linked list.  If 'room' is
     * RO_ROOM_NONE or the object isn't found, has no effect.
     *
     * Uses a bit vector to memoize all objects we've visited, so we
     * can exit if there's a cycle in the list. This may happen if we
     * are operating on world data which isn't yet initialized.
     */

    if (room == RO_ROOM_NONE) {
        return;
    }

    std::vector<bool> memo(256, false);

    uint8_t *head = &rooms.objectListHead[room];

    while (*head != RO_OBJ_NONE) {

        if (memo[*head]) {
            /* Already been here! */
            return;
        }
        memo[*head] = true;

        if (*head == obj) {
            /* Found it */
            *head = objects.nextInRoom[obj];
            return;
        }
        head = &objects.nextInRoom[*head];
    }
}

void ROWorld::addObjectToRoom(ROObjectId obj, RORoomId room)
{
    /*
     * Add an object to a room's linked list.
     * If 'room' is RO_ROOM_NONE, has no effect.
     */

    if (room == RO_ROOM_NONE) {
        return;
    }

    objects.nextInRoom[obj] = rooms.objectListHead[room];
    rooms.objectListHead[room] = obj;
}

void ROWorld::setRobotRoom(ROObjectId obj, RORoomId room)
{
    /*
     * Robots come in two halves. Set both of them to the same room.
     */

    ROObjectId left = (ROObjectId) (obj & ~1);
    ROObjectId right = (ROObjectId) (left + 1);

    setObjectRoom(left, room);
    setObjectRoom(right, room);
}

void ROWorld::setRobotXY(ROObjectId obj, int x, int y)
{
    /*
     * Robots come in two halves. Set the left one to the given
     * position, and the right one needs to be 5 pixels away from the
     * left one.
     */

    ROObjectId left = (ROObjectId) (obj & ~1);
    ROObjectId right = (ROObjectId) (left + 1);

    setObjectXY(left, x, y);
    setObjectXY(right, x + 5, y);
}

ROCircuit *ROCircuit::fromProcess(SBTProcess *proc)
{
    int addr = proc->getAddress(SBTADDR_CIRCUIT_DATA);
    return addr < 0 ? 0 : reinterpret_cast<ROCircuit*>(proc->memSeg(proc->reg.ds) + addr);
}

RORobot *RORobot::fromProcess(SBTProcess *proc)
{
    int addr = proc->getAddress(SBTADDR_ROBOT_DATA_MAIN);
    return addr < 0 ? 0 : reinterpret_cast<RORobot*>(proc->memSeg(proc->reg.ds) + addr);
}

RORobotGrabber *RORobotGrabber::fromProcess(SBTProcess *proc)
{
    int addr = proc->getAddress(SBTADDR_ROBOT_DATA_GRABBER);
    return addr < 0 ? 0 : reinterpret_cast<RORobotGrabber*>(proc->memSeg(proc->reg.ds) + addr);
}

const char *ROSavedGame::getWorldName()
{
    switch (worldId) {
        case RO_WORLD_SEWER:   return "City Sewer";
        case RO_WORLD_SUBWAY:  return "The Subway";
        case RO_WORLD_TOWN:    return "Streets of Robotropolis";
        case RO_WORLD_COMP:    return "Master Computer Center";
        case RO_WORLD_STREET:  return "The Skyways";
        case RO_WORLD_LAB:     return "Saved Lab";
        default:               return "(Unknown)";
    }
}

const char *ROSavedGame::getProcessName()
{
    // Which process do we load this world as?
    switch (worldId) {
        case RO_WORLD_SEWER:
        case RO_WORLD_SUBWAY:
        case RO_WORLD_TOWN:
        case RO_WORLD_COMP:
        case RO_WORLD_STREET:
            return "game.exe";

        case RO_WORLD_LAB:
            return "lab.exe";
    }
    return 0;
}

bool ROData::fromProcess(SBTProcess *proc)
{
    world = ROWorld::fromProcess(proc);
    circuit = ROCircuit::fromProcess(proc);
    robots.state = RORobot::fromProcess(proc);
    robots.grabbers = RORobotGrabber::fromProcess(proc);

    if (!world || !circuit || !robots.state || !robots.grabbers) {
        return false;
    }

    /*
     * We can infer the number of robots by looking at the
     * size of the robotGrabbers table, which is just prior
     * to the 'robots' table.
     *
     * Also, we can double-check this by looking for an 0xFF
     * termination byte just after the end of the RORobot table.
     */

    robots.count = (RORobotGrabber*)robots.state - robots.grabbers;
    assert((robots.count == 3 || robots.count == 4) && "Robot table sanity check failed");

    uint8_t *endOfTable = (uint8_t*)&robots.state[robots.count];
    assert(*endOfTable == 0xFF && "End of robot table not found");

    /*
     * Just after the robot's table terminator, there is an array of
     * robot battery accumulators.
     */

    robots.batteryAcc = (RORobotBatteryAcc*) (endOfTable + 1);

    return true;
}

void ROData::copyFrom(ROData *source)
{
    /*
     * Copy all world data from another process.
     */

    if (this == source) {
        return;
    }

    const int copyRobots = robots.count < source->robots.count ?
                           robots.count : source->robots.count;

    memcpy(world, source->world, sizeof *world);
    memcpy(circuit, source->circuit, sizeof *circuit);

    memcpy(robots.grabbers, source->robots.grabbers,
           sizeof robots.grabbers[0] * copyRobots);
    memcpy(robots.state, source->robots.state,
           sizeof robots.state[0] * copyRobots);
    memcpy(robots.batteryAcc, source->robots.batteryAcc,
           sizeof robots.batteryAcc[0] * copyRobots);

    /*
     * Move the grabber sprites, if necessary.
     */

    if (robots.count == 4 && source->robots.count == 3) {
        /* Convert sprites from TUT/LAB to GAME IDs */
        memcpy(&world->sprites[RO_SPR_GAME_GRABBER_UP],
               &source->world->sprites[RO_SPR_GRABBER_UP], sizeof(ROSprite));
        memcpy(&world->sprites[RO_SPR_GAME_GRABBER_RIGHT],
               &source->world->sprites[RO_SPR_GRABBER_RIGHT], sizeof(ROSprite));
        memcpy(&world->sprites[RO_SPR_GAME_GRABBER_LEFT],
               &source->world->sprites[RO_SPR_GRABBER_LEFT], sizeof(ROSprite));
    } else if (robots.count == 3 && source->robots.count == 4) {
        /* Convert sprites from GAME to TUT/LAB IDs */
        memcpy(&world->sprites[RO_SPR_GRABBER_UP],
               &source->world->sprites[RO_SPR_GAME_GRABBER_UP], sizeof(ROSprite));
        memcpy(&world->sprites[RO_SPR_GRABBER_RIGHT],
               &source->world->sprites[RO_SPR_GAME_GRABBER_RIGHT], sizeof(ROSprite));
        memcpy(&world->sprites[RO_SPR_GRABBER_LEFT],
               &source->world->sprites[RO_SPR_GAME_GRABBER_LEFT], sizeof(ROSprite));
    }
}

ROJoyfile::ROJoyfile()
{
    // Start out empty
    memset(this, 0, sizeof *this);

    // These values seem to be factory defaults
    joystick_io_port = DEFAULT_JOYSTICK_PORT;
    debug_control = DEBUG_NORMAL;
    disk_drive_id = DRIVE_A;

    // Enable joystick support with generic values
    joystick_enabled = true;
    x_center = y_center = DEFAULT_JOYSTICK_CENTER;
    xplus_divisor = yplus_divisor = DEFAULT_JOYSTICK_DIVISOR;
    xminus_divisor = yminus_divisor = DEFAULT_JOYSTICK_DIVISOR;
}

void ROJoyfile::setCheatsEnabled(bool enable)
{
    cheat_control = enable ? CHEATS_ENABLED : 0;
}
