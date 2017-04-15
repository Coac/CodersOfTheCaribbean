#pragma GCC optimize("-O3")
#pragma GCC optimize("inline")
#pragma GCC optimize("omit-frame-pointer")
#pragma GCC optimize("unroll-loops")

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <math.h>

using namespace std;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

enum EntityType {
    SHIP, BARREL, MINE, CANNONBALL
};

class Coord {
private:
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

public:
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

    int distanceTo(Coord *coord) {
        return abs(this->getX() - coord->getX()) + abs(this->getY() - coord->getY());
    }
};

class Entity {
private:
    int id;
    EntityType type;
    Coord *position;

public:
    Entity(EntityType type, int id, int x, int y) {
        this->id = 0;
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

    Entity *getClosest(const vector<Entity *> &entities, int *closestDistance = nullptr) const {
        int min = 999;
        Entity *closest = nullptr;
        for (auto &entity : entities) {
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
};

class Ship : public Entity {
private:
    int orientation;
    int speed;
    int health;
    int owner;
    int cannonCooldown;
    int mineCooldown;

public:
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
    vector<RumBarrel *> rumBarrels;
    vector<Ship *> allyShips;
    vector<Ship *> enemyShips;

    void parseInputs() {
        rumBarrels.clear();
        enemyShips.clear();

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
                    auto oldShip = find_if(allyShips.begin(), allyShips.end(), [&](Ship *o) {
                        return o->getId() == ship->getId();
                    });
                    if (oldShip == allyShips.end()) {
                        allyShips.push_back(ship);
                    } else {
                        (*oldShip)->update(*ship);
                    }
                } else {
                    enemyShips.push_back(ship);
                }
            } else if (entityType == "BARREL") {
                rumBarrels.push_back(new RumBarrel(entityId, x, y, arg1));
            }

        }
    }

    void decrementCooldown() {
        for (auto &ship : allyShips) {
            ship->decrementCooldown();
        }
        for (auto &ship : enemyShips) {
            ship->decrementCooldown();
        }
    }

    void sendOutputs() {
        for (auto &ship : allyShips) {
            vector<Entity *> barrels(rumBarrels.begin(), rumBarrels.end());
            Entity *closestBarrel = ship->getClosest(barrels);

            vector<Entity *> enemies(enemyShips.begin(), enemyShips.end());
            int enemyDist = 999;
            Ship *closestEnemy = (Ship *) ship->getClosest(enemies, &enemyDist);

            if (enemyDist < 10 && !ship->isCannonOnCd()) {
                Coord *coord = closestEnemy->getPosition()->neighbor(closestEnemy->getOrientation());
                ship->fire(coord->getX(), coord->getY());
            } else {
                cout << "MOVE " << closestBarrel->getPosition()->getX() << " " << closestBarrel->getPosition()->getY()
                     << endl;
            }

        }
    }
};

int main() {

    GameState *state = new GameState();

    while (1) {
        state->parseInputs();
        state->decrementCooldown();
        state->sendOutputs();
    }
}

#pragma clang diagnostic pop
