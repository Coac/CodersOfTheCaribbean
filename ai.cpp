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

    Coord *getPosition() const {
        return position;
    }

    int distanceTo(Entity entity) {
        return this->position->distanceTo(entity.getPosition());
    }

    Entity *getClosest(const vector<Entity *> &entities, int *closestDistance = nullptr) {
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

public:
    Ship(int id, int x, int y, int orientation, int speed, int health, int owner) : Entity(SHIP, id, x, y) {
        this->orientation = orientation;
        this->speed = speed;
        this->health = health;
        this->owner = owner;
    }

    bool isAlly() {
        return owner == 1;
    }
};

class RumBarrel : public Entity {
private:
    int health;

public:
    RumBarrel(int id, int x, int y, int health) : Entity(SHIP, id, x, y) {
        this->health = health;
    }
};


class GameState {
public:
    vector<RumBarrel *> rumBarrels;
    vector<Ship *> allyShips;
    vector<Ship *> enemyShips;

    void parseInputs() {
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
                    allyShips.push_back(ship);
                } else {
                    enemyShips.push_back(ship);
                }
            } else if (entityType == "BARREL") {
                rumBarrels.push_back(new RumBarrel(entityId, x, y, arg1));
            }

        }
    }

    void sendOutputs() {
        for (auto &ship : allyShips) {
            vector<Entity *> barrels(rumBarrels.begin(), rumBarrels.end());
            Entity *closestBarrel = ship->getClosest(barrels);

            vector<Entity *> enemies(enemyShips.begin(), enemyShips.end());
            int enemyDist = 999;
            Entity *closestEnemy = ship->getClosest(enemies, &enemyDist);
            cerr << enemyDist;

            if(enemyDist < 10) {
                cout << "FIRE " << closestEnemy->getPosition()->getX() << " " << closestEnemy->getPosition()->getY()
                     << endl;
            } else {
                cout << "MOVE " << closestBarrel->getPosition()->getX() << " " << closestBarrel->getPosition()->getY()
                     << endl;
            }

        }
    }
};

int main() {

    while (1) {
        GameState *state = new GameState();
        state->parseInputs();
        state->sendOutputs();
    }
}

#pragma clang diagnostic pop
