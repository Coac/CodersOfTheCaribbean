#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

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
};

class Ship : Entity {
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

class RumBarrel : Entity {
private:
    int health;

public:
    RumBarrel(int id, int x, int y, int health) : Entity(SHIP, id, x, y) {
        this->health = health;
    }
};


int main() {

    while (1) {
        vector<RumBarrel *> rumBarrels;
        vector<Ship *> allyShips;
        vector<Ship *> enemyShips;

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
        for (int i = 0; i < myShipCount; i++) {

            // Write an action using cout. DON'T FORGET THE "<< endl"
            // To debug: cerr << "Debug messages..." << endl;

            cout << "MOVE 11 10" << endl; // Any valid action, such as "WAIT" or "MOVE x y"
        }
    }
}

#pragma clang diagnostic pop
