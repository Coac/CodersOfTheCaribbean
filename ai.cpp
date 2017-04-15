#pragma GCC optimize("-O3")
#pragma GCC optimize("inline")
#pragma GCC optimize("omit-frame-pointer")
#pragma GCC optimize("unroll-loops")

#include <iostream>
#include <algorithm>
#include <math.h>
#include <chrono>

using namespace std;
using namespace std::chrono;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"


const int MAP_WIDTH = 23;
const int MAP_HEIGHT = 21;
const int MAX_RUM_BARRELS = 26;

class RumBarrel;

enum EntityType {
    SHIP, BARREL, MINE, CANNONBALL
};

class CubeCoordinate {
public:
    int x;
    int y;
    int z;

    int directions[6][3] = {{1,  -1, 0},
                            {+1, 0,  -1},
                            {0,  +1, -1},
                            {-1, +1, 0},
                            {-1, 0,  +1},
                            {0,  -1, +1}};

    CubeCoordinate(int x, int y, int z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    int distanceTo(CubeCoordinate *dst) {
        return (abs(x - dst->x) + abs(y - dst->y) + abs(z - dst->z)) / 2;
    }
};

class Coord {
public:
    int x;
    int y;

    int DIRECTIONS_EVEN[6][2] = {{1,  0},
                                 {0,  -1},
                                 {-1, -1},
                                 {-1, 0},
                                 {-1, 1},
                                 {0,  1}};
    int DIRECTIONS_ODD[6][2] = {{1,  0},
                                {1,  -1},
                                {0,  -1},
                                {-1, 0},
                                {0,  1},
                                {1,  1}};

    Coord(int x, int y) {
        this->x = x;
        this->y = y;
    }

    int getY() const {
        return this->y;
    }

    void setY(int y) {
        this->y = y;
    }

    int getX() const {
        return this->x;
    }

    void setX(int x) {
        this->x = x;
    }

    bool isInsideMap() {
        return x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT;
    }

    Coord *neighbor(int orientation) {
        int newY, newX;
        if (this->y % 2 == 1) {
            newY = this->y + DIRECTIONS_ODD[orientation][1];
            newX = this->x + DIRECTIONS_ODD[orientation][0];
        } else {
            newY = this->y + DIRECTIONS_EVEN[orientation][1];
            newX = this->x + DIRECTIONS_EVEN[orientation][0];
        }

        return new Coord(newX, newY);
    }

    Coord *neighbor(int orientation, int distance) {
        Coord *coord = this->neighbor(orientation);
        for (int i = 0; i < distance - 1; ++i) {
            coord = coord->neighbor(orientation);
        }
        return coord;
    }

    CubeCoordinate *toCubeCoordinate() {
        int xp = x - (y - (y & 1)) / 2;
        int zp = y;
        int yp = -(xp + zp);
        return new CubeCoordinate(xp, yp, zp);
    }

    int distanceTo(Coord *dst) {
        return this->toCubeCoordinate()->distanceTo(dst->toCubeCoordinate());
    }
};

class Entity {
public:
    int id;
    EntityType type;
    Coord *position;

    Entity(EntityType type, int id, int x, int y) {
        this->id = id;
        this->type = type;
        this->position = new Coord(x, y);
    }

    int getId() const {
        return this->id;
    }

    Coord *getPosition() const {
        return position;
    }

    int distanceTo(Entity entity) const {
        return this->position->distanceTo(entity.getPosition());
    }

    Entity *getClosestEntity(Entity **entities, int entityCount, int *closestDistance = nullptr) const {
        int min = 999;
        Entity *closest = nullptr;
        for (int i = 0; i < entityCount; ++i) {
            Entity *entity = entities[i];
            int dist = this->distanceTo(*entity);
            if (dist < min) {
                min = dist;
                closest = entity;
            }
        }
        if (closestDistance) {
            *closestDistance = min;
        }
        return closest;
    }

    static Entity *getClosestEntity(Entity **entities, int entityCount, Coord *coord, int *closestDistance = nullptr) {
        int min = 999;
        Entity *closest = nullptr;
        for (int i = 0; i < entityCount; ++i) {
            Entity *entity = entities[i];
            int dist = coord->distanceTo(entity->getPosition());
            if (dist < min) {
                min = dist;
                closest = entity;
            }
        }
        if (closestDistance) {
            *closestDistance = min;
        }
        return closest;
    }

    static Entity *findById(Entity **entities, int entityCount, int id) {
        for (int i = 0; i < entityCount; ++i) {
            if (entities[i]->getId() == id) {
                return entities[i];
            }
        }
        return nullptr;
    }
};

class Ship : public Entity {
public:
    int orientation;
    int speed;
    int health;
    int owner;
    int cannonCooldown;
    int mineCooldown;

    Ship(int id, int x, int y, int orientation, int speed, int health, int owner) : Entity(SHIP, id, x, y) {
        this->orientation = orientation;
        this->speed = speed;
        this->health = health;
        this->owner = owner;
        this->cannonCooldown = 0;
        this->mineCooldown = 0;
    }

    bool isAlly() {
        return owner == 1;
    }

    int getOrientation() {
        return orientation;
    }

    void update(const Ship ship) {
        this->orientation = ship.orientation;
        this->speed = ship.speed;
        this->health = ship.health;
        this->owner = ship.owner;
    }

    void decrementCooldown() {
        --cannonCooldown;
        --mineCooldown;
    }

    bool isMineOnCd() {
        return mineCooldown > 0;
    }

    bool isCannonOnCd() {
        return cannonCooldown > 0;
    }

    Coord *stern() {
        return position->neighbor((orientation + 3) % 6);
    }

    Coord *bow() {
        return position->neighbor(orientation);
    }

    void fire(int x, int y) {
        cannonCooldown = 2;
        cout << "FIRE " << x << " " << y
             << endl;
    }
};

class RumBarrel : public Entity {
private:
    int health;

public:
    RumBarrel(int id, int x, int y, int health) : Entity(BARREL, id, x, y) {
        this->health = health;
    }
};


class GameState {
public:
    RumBarrel *rumBarrels[MAX_RUM_BARRELS];
    int rumBarrelCount = 0;

    Ship *allyShips[3];
    int allyShipCount = 0;

    Ship *enemyShips[3];
    int enemyShipCount = 0;

    void parseInputs() {

        // Maybe free
        rumBarrelCount = 0;
        enemyShipCount = 0;

        int myShipCount; // the number of remaining ships
        cin >> myShipCount;
        cin.ignore();
        int entityCount; // the number of entities (e.g. ships, mines or cannonballs)
        cin >> entityCount;
        cin.ignore();
        for (int i = 0; i < entityCount; i++) {
            int entityId;
            string entityType;
            int x;
            int y;
            int arg1;
            int arg2;
            int arg3;
            int arg4;
            cin >> entityId >> entityType >> x >> y >> arg1 >> arg2 >> arg3 >> arg4;
            cin.ignore();

            if (entityType == "SHIP") {
                Ship *ship = new Ship(entityId, x, y, arg1, arg2, arg3, arg4);
                if (ship->isAlly()) {
                    auto oldShip = (Ship *) Entity::findById((Entity **) allyShips, allyShipCount, entityId);
                    if (oldShip == nullptr) {
                        allyShips[allyShipCount] = ship;
                        ++allyShipCount;
                    } else {
                        oldShip->update(*ship);
                    }
                } else {
                    enemyShips[enemyShipCount] = ship;
                    ++enemyShipCount;
                }
            } else if (entityType == "BARREL") {
                rumBarrels[rumBarrelCount] = new RumBarrel(entityId, x, y, arg1);
                ++rumBarrelCount;
            }

        }
    }

    void decrementCooldown() {
        for (int i = 0; i < allyShipCount; ++i) {
            allyShips[i]->decrementCooldown();
        }

        for (int i = 0; i < enemyShipCount; ++i) {
            enemyShips[i]->decrementCooldown();
        }
    }

    void sendOutputs() {
        for (int i = 0; i < allyShipCount; ++i) {
            Ship *ship = allyShips[i];

            Entity *closestBarrel = ship->getClosestEntity((Entity **) rumBarrels, rumBarrelCount);

            int enemyDist = 999;

            Ship *closestEnemy = (Ship *) Entity::getClosestEntity((Entity **) enemyShips, enemyShipCount, ship->bow(),
                                                                  &enemyDist);

            cerr << ship->getId() << " " << closestEnemy->getId() << " dist:" << enemyDist << endl;
            if (enemyDist < 10 && !ship->isCannonOnCd()) {
                Coord *coord;
                if (closestEnemy->speed == 0) {
                    coord = closestEnemy->getPosition();
                } else if (closestEnemy->speed == 1) {
                    coord = closestEnemy->getPosition()->neighbor(closestEnemy->getOrientation(),
                                                                  1 + (enemyDist / 3));
                } else {
                    coord = closestEnemy->getPosition()->neighbor(closestEnemy->getOrientation(),
                                                                  2 + (enemyDist / 3));
                }

                if (!coord->isInsideMap()) {
                    coord = closestEnemy->getPosition();
                }
                ship->fire(coord->getX(), coord->getY());
            } else {
                if (closestBarrel == nullptr) {
                    cout << "MOVE " << closestEnemy->getPosition()->y << " " << closestEnemy->getPosition()->y << endl;
                } else {
                    cout << "MOVE " << closestBarrel->getPosition()->getX() << " "
                         << closestBarrel->getPosition()->getY() << endl;
                }

            }

        }
    }
};

int main() {

    GameState *state = new GameState();

    while (1) {
        high_resolution_clock::time_point t1 = high_resolution_clock::now();

        state->parseInputs();
        state->decrementCooldown();
        state->sendOutputs();

        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(t2 - t1).count();
        cerr << duration << " ms";
    }
}

#pragma clang diagnostic pop
