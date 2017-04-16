#pragma GCC optimize("-O3")
#pragma GCC optimize("inline")
#pragma GCC optimize("omit-frame-pointer")
#pragma GCC optimize("unroll-loops")

#include <iostream>
#include <algorithm>
#include <math.h>
#include <chrono>
#include <assert.h>

using namespace std;
using namespace std::chrono;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"


const int MAP_WIDTH = 23;
const int MAP_HEIGHT = 21;
const int COOLDOWN_CANNON = 2;
const int COOLDOWN_MINE = 5;
const int INITIAL_SHIP_HEALTH = 100;
const int MAX_SHIP_HEALTH = 100;
const int MAX_SHIP_SPEED = 2;
const int MAX_SHIPS = 3;
const int MAX_MINES = 10;
const int MIN_RUM_BARRELS = 10;
const int MAX_RUM_BARRELS = 26;
const int MIN_RUM_BARREL_VALUE = 10;
const int MAX_RUM_BARREL_VALUE = 20;
const int REWARD_RUM_BARREL_VALUE = 30;
const int MINE_VISIBILITY_RANGE = 5;
const int FIRE_DISTANCE_MAX = 10;
const int LOW_DAMAGE = 25;
const int HIGH_DAMAGE = 50;
const int MINE_DAMAGE = 25;
const int NEAR_MINE_DAMAGE = 10;


class RumBarrel;

template<typename T, int N>
class List {
public:
    T array[N + 1];
    int count = 0;

    void add(T element) {
        array[count] = element;
        ++count;
    }

    void clear() {
        count = 0;
    }

    void removeAt(int index) {
        for (int i = index; i < count - 1; ++i) {
            array[i] = array[i + 1];
        }
        --count;
    }

    T *begin() { return &array[0]; }

    T *end() {
        return &array[count];
    }

};

enum EntityType {
    SHIP, BARREL, MINE, CANNONBALL
};

enum Action {
    FASTER, SLOWER, PORT, STARBOARD, FIRE, MOVE
};

class CubeCoordinate {
public:
    int x;
    int y;
    int z;

    static int directions[6][3];

    CubeCoordinate(int x, int y, int z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    int distanceTo(CubeCoordinate *dst) {
        return (abs(x - dst->x) + abs(y - dst->y) + abs(z - dst->z)) / 2;
    }
};

int CubeCoordinate::directions[6][3] = {{1,  -1, 0},
                                        {+1, 0,  -1},
                                        {0,  +1, -1},
                                        {-1, +1, 0},
                                        {-1, 0,  +1},
                                        {0,  -1, +1}};

class Coord {
public:
    int x;
    int y;

    static int DIRECTIONS_EVEN[6][2];
    static int DIRECTIONS_ODD[6][2];

    Coord(int x, int y) {
        this->x = x;
        this->y = y;
    }

    Coord() {
        this->x = -1;
        this->y = -1;
    }

    bool equals(Coord other) {
        return x == other.x && y == other.y;
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

    Coord neighbor(int orientation) {
        int newY, newX;
        if (this->y % 2 == 1) {
            newY = this->y + DIRECTIONS_ODD[orientation][1];
            newX = this->x + DIRECTIONS_ODD[orientation][0];
        } else {
            newY = this->y + DIRECTIONS_EVEN[orientation][1];
            newX = this->x + DIRECTIONS_EVEN[orientation][0];
        }

        Coord coord;
        coord.x = newX;
        coord.y = newY;

        return coord;
    }

    Coord neighbor(int orientation, int distance) {
        Coord coord = this->neighbor(orientation);
        for (int i = 0; i < distance - 1; ++i) {
            coord = coord.neighbor(orientation);
        }
        return coord;
    }

    CubeCoordinate *toCubeCoordinate() {
        int xp = x - (y - (y & 1)) / 2;
        int zp = y;
        int yp = -(xp + zp);
        return new CubeCoordinate(xp, yp, zp);
    }

    int distanceTo(Coord dst) {
        return this->toCubeCoordinate()->distanceTo(dst.toCubeCoordinate());
    }
};

int Coord::DIRECTIONS_EVEN[6][2] = {{1,  0},
                                    {0,  -1},
                                    {-1, -1},
                                    {-1, 0},
                                    {-1, 1},
                                    {0,  1}};
int Coord::DIRECTIONS_ODD[6][2] = {{1,  0},
                                   {1,  -1},
                                   {0,  -1},
                                   {-1, 0},
                                   {0,  1},
                                   {1,  1}};

class Entity {
public:
    int id;
    EntityType type;
    Coord position;

    Entity(EntityType type, int id, int x, int y) {
        this->id = id;
        this->type = type;
        this->position.x = x;
        this->position.y = y;
    }

    int getId() const {
        return this->id;
    }

    Coord getPosition() const {
        return position;
    }

    int distanceTo(Entity entity) {
        return this->position.distanceTo(entity.getPosition());
    }

    Entity *getClosestEntity(Entity **entities, int entityCount, int *closestDistance = nullptr) {
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

    static Entity *getClosestEntity(Entity **entities, int entityCount, Coord coord, int *closestDistance = nullptr) {
        int min = 999;
        Entity *closest = nullptr;
        for (int i = 0; i < entityCount; ++i) {
            Entity *entity = entities[i];
            int dist = coord.distanceTo(entity->getPosition());
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
    int newOrientation;
    int speed;
    int health;
    int owner;
    int cannonCooldown;
    int mineCooldown;
    Coord newPosition;
    Coord newBowCoordinate;
    Coord newSternCoordinate;
    bool isDead = false;


    Action action;
    int targetX;
    int targetY;

    Ship(int id, int x, int y, int orientation, int speed, int health, int owner) : Entity(SHIP, id, x, y) {
        this->orientation = orientation;
        this->speed = speed;
        this->health = health;
        this->owner = owner;
        this->cannonCooldown = 0;
        this->mineCooldown = 0;
        this->action = FASTER;
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

        this->position = ship.position;
        this->isDead = false;
    }

    void decrementCooldown() {
        --cannonCooldown;
        --mineCooldown;
    }

    bool newBowIntersect(Ship *other) {
        return newBowCoordinate.x != -1 && newBowCoordinate.y != -1 &&
               (newBowCoordinate.equals(other->newBowCoordinate) || newBowCoordinate.equals(other->newPosition)
                || newBowCoordinate.equals(other->newSternCoordinate));
    }

    bool newBowIntersect(List<Ship *, 6> ships) {
        for (auto other : ships) {
            if (this != other && newBowIntersect(other)) {
                return true;
            }
        }
        return false;
    }

    bool newPositionsIntersect(Ship *other) {
        bool sternCollision = newSternCoordinate.x != -1 && (newSternCoordinate.equals(other->newBowCoordinate)
                                                             || newSternCoordinate.equals(other->newPosition) ||
                                                             newSternCoordinate.equals(other->newSternCoordinate));
        bool centerCollision = newPosition.x != -1 &&
                               (newPosition.equals(other->newBowCoordinate) || newPosition.equals(other->newPosition)
                                || newPosition.equals(other->newSternCoordinate));
        return newBowIntersect(other) || sternCollision || centerCollision;
    }

    bool newPositionsIntersect(List<Ship *, 6> ships) {
        for (auto other : ships) {
            if (this != other && newPositionsIntersect(other)) {
                return true;
            }
        }
        return false;
    }


    bool isMineOnCd() {
        return mineCooldown > 0;
    }

    bool isCannonOnCd() {
        return cannonCooldown > 0;
    }

    Coord stern() {
        return position.neighbor((orientation + 3) % 6);
    }

    Coord bow() {
        return position.neighbor(orientation);
    }

    Coord newStern() {
        return position.neighbor((newOrientation + 3) % 6);
    }

    Coord newBow() {
        return position.neighbor(newOrientation);
    }

    void fire(int x, int y) {
        this->action = FIRE;
        this->targetX = x;
        this->targetY = y;
    }

    void faster() {
        this->action = Action::FASTER;
    }

    void slower() {
        this->action = Action::SLOWER;
    }

    void port() {
        this->action = Action::PORT;
    }

    void starboard() {
        this->action = Action::STARBOARD;
    }

    void move(int x, int y) {
        this->action = MOVE;
        this->targetX = x;
        this->targetY = y;
    }

    void sendOutput() {
        switch (this->action) {
            case Action::FASTER:
                cout << "FASTER" << endl;
                break;
            case Action::SLOWER:
                cout << "SLOWER" << endl;
                break;
            case Action::STARBOARD:
                cout << "STARBOARD" << endl;
                break;
            case Action::PORT:
                cout << "PORT" << endl;
                break;
            case Action::FIRE:
                cout << "FIRE " << this->targetX << " " << this->targetY << endl;
                cannonCooldown = 2;
                break;
            case Action::MOVE:
                cout << "MOVE " << this->targetX << " " << this->targetY << endl;
                break;
        }

    }
};

class RumBarrel : public Entity {
public:
    int health;

    RumBarrel(int id, int x, int y, int health) : Entity(BARREL, id, x, y) {
        this->health = health;
    }

    RumBarrel *clone() {
        return new RumBarrel(id, this->position.x, this->position.y, health);
    }
};

class Mine : public Entity {
public:
    Mine(int id, int x, int y) : Entity(MINE, id, x, y) {
    }
};

class CannonBall : public Entity {
public:
    int ownerEntityId;
    int remainingTurns;

    CannonBall(int id, int x, int y, int ownerEntityId, int remainingTurns) : Entity(CANNONBALL, id, x, y) {
        this->ownerEntityId = ownerEntityId;
        this->remainingTurns = remainingTurns;
    }
};

class GameState {
public:
    List<RumBarrel *, MAX_RUM_BARRELS> rumBarrels;

    List<Ship *, 3> allyShips;

    List<Ship *, 3> enemyShips;

    List<Ship *, 6> ships;

    List<Mine *, MAX_MINES> mines;

    List<CannonBall *, 100> cannonBalls;

    List<Coord, 100> cannonBallExplosions;

    GameState *clone() {
        GameState *cloned = new GameState();
        cloned->rumBarrels.count = this->rumBarrels.count;
        cloned->allyShips.count = this->allyShips.count;
        cloned->enemyShips.count = this->enemyShips.count;
        cloned->mines.count = this->mines.count;
        cloned->cannonBalls.count = this->cannonBalls.count;

        for (int i = 0; i < rumBarrels.count; ++i) {
            cloned->rumBarrels.array[i] = this->rumBarrels.array[i]->clone();
        }
        for (int i = 0; i < allyShips.count; ++i) {
            cloned->allyShips.array[i] = this->allyShips.array[i];
        }
        for (int i = 0; i < enemyShips.count; ++i) {
            cloned->enemyShips.array[i] = this->enemyShips.array[i];
        }
        for (int i = 0; i < ships.count; ++i) {
            cloned->ships.array[i] = this->ships.array[i];
        }
        for (int i = 0; i < mines.count; ++i) {
            cloned->mines.array[i] = this->mines.array[i];
        }
        for (int i = 0; i < cannonBalls.count; ++i) {
            cloned->cannonBalls.array[i] = this->cannonBalls.array[i];
        }

        return cloned;
    }

    ~GameState() {
        for (int i = 0; i < rumBarrels.count; ++i) {
            delete this->rumBarrels.array[i];
        }
    }

    void moveCannonballs() {
        for (int i = 0; i < cannonBalls.count; ++i) {
            CannonBall *ball = cannonBalls.array[i];

            if (ball->remainingTurns == 0) {
                cannonBalls.removeAt(i);
                continue;
            } else if (ball->remainingTurns > 0) {
                ball->remainingTurns--;
            }

            if (ball->remainingTurns == 0) {
                cannonBallExplosions.add(ball->position);
            }
        }
    }

    void decrementRum() {
        for (auto ship : allyShips) {
            --ship->health;
        }
        for (auto ship : enemyShips) {
            --ship->health;
        }
    }

    void applyActions() {
        for (auto ship : ships) {
            if (ship->isDead) continue;

            if (ship->mineCooldown > 0) {
                ship->mineCooldown--;
            }
            if (ship->cannonCooldown > 0) {
                ship->cannonCooldown--;
            }

            ship->newOrientation = ship->orientation;

            switch (ship->action) {
                case FASTER:
                    if (ship->speed < MAX_SHIP_SPEED) {
                        ship->speed++;
                    }
                    break;
                case SLOWER:
                    if (ship->speed > 0) {
                        ship->speed--;
                    }
                    break;
                case PORT:
                    ship->newOrientation = (ship->orientation + 1) % 6;
                    break;
                case STARBOARD:
                    ship->newOrientation = (ship->orientation + 5) % 6;
                    break;
//                case MINE:
//                    if (ship->mineCooldown == 0) {
//                        Coord target = ship->stern().neighbor((ship->orientation + 3) % 6);
//
//                        if (target.isInsideMap()) {
//                            bool cellIsFreeOfBarrels = barrels.stream().noneMatch(
//                                    barrel->barrel.position.equals(target));
//                            bool cellIsFreeOfMines = mines.stream().noneMatch(mine->mine.position.equals(target));
//                            bool cellIsFreeOfShips = ships.stream().filter(b->b != ship).noneMatch(b->b.at(target));
//
//                            if (cellIsFreeOfBarrels && cellIsFreeOfShips && cellIsFreeOfMines) {
//                                ship->mineCooldown = COOLDOWN_MINE;
//                                Mine mine = new Mine(target.x, target.y);
//                                mines.add(mine);
//                            }
//                        }
//
//                    }
//                    break;
                case FIRE:
                    Coord target;
                    target.x = ship->targetX;
                    target.y = ship->targetY;

                    int distance = ship->bow().distanceTo(target);
                    if (target.isInsideMap() && distance <= FIRE_DISTANCE_MAX && ship->cannonCooldown == 0) {
                        int travelTime = (int) (1 + round(ship->bow().distanceTo(target) / 3.0));
                        cannonBalls.add(new CannonBall(9999, target.x, target.y, ship->id, travelTime));
                        ship->cannonCooldown = COOLDOWN_CANNON;
                    }
                    break;
            }

        }
    }

    void moveShips() {
        // ---
        // Go forward
        // ---
        for (int i = 1; i <= MAX_SHIP_SPEED; i++) {
            for (auto ship : ships) {
                if (ship->isDead) continue;

                ship->newPosition = ship->position;
                ship->newBowCoordinate = ship->bow();
                ship->newSternCoordinate = ship->stern();

                if (i > ship->speed) {
                    continue;
                }

                Coord newCoordinate = ship->position.neighbor(ship->orientation);

                if (newCoordinate.isInsideMap()) {
                    // Set new coordinate.
                    ship->newPosition = newCoordinate;
                    ship->newBowCoordinate = newCoordinate.neighbor(ship->orientation);
                    ship->newSternCoordinate = newCoordinate.neighbor((ship->orientation + 3) % 6);
                } else {
                    // Stop ship!
                    ship->speed = 0;
                }
            }

            // Check ship and obstacles collisions
            List<Ship *, 10> collisions;
            bool collisionDetected = true;
            while (collisionDetected) {
                collisionDetected = false;

                for (auto ship : ships) {
                    if (ship->isDead) continue;

                    if (ship->newBowIntersect(ships)) {
                        collisions.add(ship);
                    }
                }

                for (auto ship : collisions) {
                    // Revert last move
                    ship->newPosition = ship->position;
                    ship->newBowCoordinate = ship->bow();
                    ship->newSternCoordinate = ship->stern();

                    // Stop ships
                    ship->speed = 0;

                    collisionDetected = true;
                }
                collisions.clear();
            }

            for (auto ship : ships) {
                if (ship->isDead) continue;

                ship->position = ship->newPosition;
                checkCollisions(ship);

            }
        }
    }

    void checkCollisions(Ship *ship) {
        Coord bow = ship->bow();
        Coord stern = ship->stern();
        Coord center = ship->position;

        // Collision with the barrels
        for (int i = 0; i < rumBarrels.count; ++i) {
            RumBarrel *barrel = rumBarrels.array[i];
            if (barrel->position.equals(bow) || barrel->position.equals(stern) || barrel->position.equals(center)) {
                ship->health += barrel->health;
                rumBarrels.removeAt(i);
            }

        }

        // Collision with the mines
        for (int i = 0; i < mines.count; ++i) {
            Mine *mine = mines.array[i];

            ship->health -= MINE_DAMAGE;
            // TODO : APROX DAMAGE

            rumBarrels.removeAt(i);
        }
    }


    void rotateShips() {
        // Rotate
        for (auto ship : ships) {
            if (ship->isDead) continue;
            ship->newPosition = ship->position;
            ship->newBowCoordinate = ship->newBow();
            ship->newSternCoordinate = ship->newStern();
        }


        // Check collisions
        bool collisionDetected = true;
        List<Ship *, 10> collisions;
        while (collisionDetected) {
            collisionDetected = false;

            for (auto ship : ships) {
                if (ship->isDead) continue;
                if (ship->newPositionsIntersect(ships)) {
                    collisions.add(ship);
                }
            }

            for (auto ship : collisions) {
                ship->newOrientation = ship->orientation;
                ship->newBowCoordinate = ship->newBow();
                ship->newSternCoordinate = ship->newStern();
                ship->speed = 0;
                collisionDetected = true;
            }

            collisions.clear();
        }

        // Apply rotation
        for (auto ship : ships) {
            if (ship->isDead) continue;
            ship->orientation = ship->newOrientation;
            checkCollisions(ship);

        }
    }


    void clearLists() {
        for (int i = 0; i < rumBarrels.count; ++i) {
            delete this->rumBarrels.array[i];
        }
        rumBarrels.clear();
        for (int i = 0; i < enemyShips.count; ++i) {
            delete this->enemyShips.array[i];
        }
        enemyShips.clear();
        for (int i = 0; i < mines.count; ++i) {
            delete this->mines.array[i];
        }
        mines.clear();
        for (int i = 0; i < cannonBalls.count; ++i) {
            delete this->cannonBalls.array[i];
        }
        cannonBalls.clear();

        ships.clear();
    }

    void parseInputs() {
        clearLists();

        for (auto ship : allyShips) {
            ship->isDead = true;
        }

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
                    auto oldShip = (Ship *) Entity::findById((Entity **) allyShips.array, allyShips.count, entityId);
                    if (oldShip == nullptr) {
                        allyShips.add(ship);
                        ships.add(ship);
                    } else {
                        oldShip->update(*ship);
                        ships.add(oldShip);
                    }
                } else {
                    enemyShips.add(ship);
                    ships.add(ship);
                }
            } else if (entityType == "BARREL") {
                rumBarrels.add(new RumBarrel(entityId, x, y, arg1));
            } else if (entityType == "MINE") {
                mines.add(new Mine(entityId, x, y));
            } else if (entityType == "CANNONBALL") {
                cannonBalls.add(new CannonBall(entityId, x, y, arg1, arg2));
            }
        }
    }

    void decrementCooldown() {
        for (auto ship : allyShips) {
            ship->decrementCooldown();
        }

        for (auto ship : enemyShips) {
            ship->decrementCooldown();
        }
    }

    void computeActions() {
        for (auto ship : allyShips) {
            if (ship->isDead) continue;

            int enemyDist = 999;
            Ship *closestEnemy = (Ship *) Entity::getClosestEntity((Entity **) enemyShips.array, enemyShips.count,
                                                                   ship->bow(),
                                                                   &enemyDist);
            cerr << ship->getId() << " " << closestEnemy->getId() << " dist:" << enemyDist << endl;
            if (enemyDist < 15 && !ship->isCannonOnCd()) {
                Coord coord;
                if (closestEnemy->speed == 0) {
                    coord = closestEnemy->getPosition();
                } else if (closestEnemy->speed == 1) {
                    coord = closestEnemy->getPosition().neighbor(closestEnemy->getOrientation(),
                                                                 1 + (enemyDist / 3));
                } else {
                    coord = closestEnemy->getPosition().neighbor(closestEnemy->getOrientation(),
                                                                 2 + (enemyDist / 3));
                }

                if (!coord.isInsideMap()) {
                    coord = closestEnemy->getPosition();
                }
                ship->fire(coord.getX(), coord.getY());
            } else {
                int barrelDist = 999;
                Entity *closestBarrel = ship->getClosestEntity((Entity **) rumBarrels.array, rumBarrels.count,
                                                               &barrelDist);
                if (closestBarrel == nullptr) {
                    ship->move(closestEnemy->getPosition().x, closestEnemy->getPosition().y);
                } else {
                    Coord shipCoord = ship->position;
                    Coord fasterCoord = shipCoord.neighbor(ship->orientation, ship->speed + 1);
                    Coord portCoord = shipCoord.neighbor(ship->orientation, ship->speed).neighbor(
                            (ship->orientation + 1) % 6);
                    Coord starboardCoord = shipCoord.neighbor(ship->orientation, ship->speed).neighbor(
                            (ship->orientation == 0 ? 5 : ship->orientation - 1));

                    int portDist = portCoord.distanceTo(closestBarrel->getPosition());
                    int starDist = starboardCoord.distanceTo(closestBarrel->getPosition());
                    int fastDist = fasterCoord.distanceTo(closestBarrel->getPosition());


                    if (fasterCoord.isInsideMap() && fastDist <= starDist && fastDist <= portDist) {
                        ship->faster();
                    } else if ((ship->speed > 0 || barrelDist == 1) && portCoord.isInsideMap() &&
                               portDist <= starDist && portDist <= fastDist) {
                        ship->port();
                    } else if ((ship->speed > 0 || barrelDist == 1) && starboardCoord.isInsideMap() &&
                               starDist <= fastDist && starDist <= portDist) {
                        ship->starboard();
                    } else {
                        ship->move(closestBarrel->getPosition().getX(), closestBarrel->getPosition().getY());
                    }

                }

            }

        }
    }

    void sendOutputs() {
        for (auto ship : allyShips) {
            if (ship->isDead) continue;

            ship->sendOutput();
        }
    }


};

int main() {

    GameState *state = new GameState();

    while (1) {
        high_resolution_clock::time_point t1 = high_resolution_clock::now();

        state->parseInputs();
        state->decrementCooldown();
        state->computeActions();
        state->sendOutputs();

        for (int i = 0; i < 20000; ++i) {
            GameState *clonedState = state->clone();

            clonedState->moveCannonballs();
            clonedState->decrementRum();

            clonedState->applyActions();
            clonedState->moveShips();
            clonedState->rotateShips();

            delete clonedState;
        }

        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(t2 - t1).count();
        cerr << duration << " ms";
    }
}

#pragma clang diagnostic pop
