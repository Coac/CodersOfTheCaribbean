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

//================================================================================
// Engine Constants
//================================================================================
constexpr int MAP_WIDTH = 23;
constexpr int MAP_HEIGHT = 21;
constexpr int COOLDOWN_CANNON = 2;
constexpr int COOLDOWN_MINE = 5;
constexpr int INITIAL_SHIP_HEALTH = 100;
constexpr int MAX_SHIP_HEALTH = 100;
constexpr int MAX_SHIP_SPEED = 2;
constexpr int MAX_SHIPS = 3;
constexpr int MAX_MINES = 10;
constexpr int MIN_RUM_BARRELS = 10;
constexpr int MAX_RUM_BARRELS = 26;
constexpr int MIN_RUM_BARREL_VALUE = 10;
constexpr int MAX_RUM_BARREL_VALUE = 20;
constexpr int REWARD_RUM_BARREL_VALUE = 30;
constexpr int MINE_VISIBILITY_RANGE = 5;
constexpr int FIRE_DISTANCE_MAX = 10;
constexpr int LOW_DAMAGE = 25;
constexpr int HIGH_DAMAGE = 50;
constexpr int MINE_DAMAGE = 25;
constexpr int NEAR_MINE_DAMAGE = 10;

//================================================================================
// AI Constants
//================================================================================
constexpr int DEPTH = 5;


class RumBarrel;

//================================================================================
// List
//================================================================================
template<typename T, int N>
class List {
public:
    T array[N + 1];
    int count = 0;

    List() {

    }

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

//================================================================================
// Enums
//================================================================================
enum EntityType {
    SHIP, BARREL, MINE, CANNONBALL, NONE
};

enum Action {
    FASTER, SLOWER, PORT, STARBOARD, WAIT, FIRE, PUTMINE, MOVE
};

//================================================================================
// CubeCoordinate
//================================================================================
class CubeCoordinate {
public:
    int x;
    int y;
    int z;

    static int directions[6][3];

    CubeCoordinate() {
        this->x = -1;
        this->y = -1;
        this->z = -1;
    }

    CubeCoordinate(int x, int y, int z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    int distanceTo(CubeCoordinate dst) {
        return (abs(x - dst.x) + abs(y - dst.y) + abs(z - dst.z)) / 2;
    }
};

int CubeCoordinate::directions[6][3] = {{1,  -1, 0},
                                        {+1, 0,  -1},
                                        {0,  +1, -1},
                                        {-1, +1, 0},
                                        {-1, 0,  +1},
                                        {0,  -1, +1}};

//================================================================================
// Coord
//================================================================================
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

    bool isBorderMap() {
        return x == 0 || y == 0 || x == MAP_WIDTH - 1 || y == MAP_HEIGHT - 1;
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

    CubeCoordinate toCubeCoordinate() {
        CubeCoordinate coord;
        int xp = x - (y - (y & 1)) / 2;
        int zp = y;
        int yp = -(xp + zp);
        coord.x = xp;
        coord.y = yp;
        coord.z = zp;
        return coord;
    }

    int distanceTo(Coord dst) {
        return this->toCubeCoordinate().distanceTo(dst.toCubeCoordinate());
    }

    friend ostream &operator<<(ostream &os, const Coord &coord);
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

ostream &operator<<(ostream &os, const Coord &coord) {
    os << "x: " << coord.x << " y: " << coord.y;
    return os;
}

//================================================================================
// Entity
//================================================================================
class Entity {
public:
    int id;
    EntityType type;
    Coord position;

    Entity() {
        type = EntityType::NONE;
        id = -1;
        position.x = -1;
        position.y = -1;
    }

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

    static Entity *findById(Entity **entities, int entityCount, int id) {
        for (int i = 0; i < entityCount; ++i) {
            if (entities[i]->getId() == id) {
                return entities[i];
            }
        }
        return nullptr;
    }
};

//================================================================================
// Ship
//================================================================================
class Ship : public Entity {
public:
    int orientation;
    int newOrientation;
    int speed;
    int health;
    int initialHealth;
    int owner;
    int cannonCooldown;
    int mineCooldown;
    Coord newPosition;
    Coord newBowCoordinate;
    Coord newSternCoordinate;
    bool isDead = false;
    bool justDied = false;

    Action action;
    int targetX;
    int targetY;

    Ship(int id, int x, int y, int orientation, int speed, int health, int owner) : Entity(SHIP, id, x, y) {
        this->orientation = orientation;
        this->speed = speed;
        this->health = health;
        this->initialHealth = health;
        this->owner = owner;
        this->cannonCooldown = 0;
        this->mineCooldown = 0;
        this->action = FASTER;

        newPosition.x = -999;
        newBowCoordinate.x = -999;
        newSternCoordinate.x = -999;

    }

    friend ostream &operator<<(ostream &os, const Ship &ship) {
        os << " orientation: " << ship.orientation << " newOrientation: "
           << ship.newOrientation << " speed: " << ship.speed << " health: " << ship.health << " owner: " << ship.owner
           << " cannonCooldown: " << ship.cannonCooldown << " mineCooldown: " << ship.mineCooldown;
        return os;
    }

    bool heal(unsigned int amount) {
        health += amount;
        if (health > MAX_SHIP_HEALTH) {
            health = MAX_SHIP_HEALTH;
        }
    }

    bool damage(unsigned int amount) {
        health -= amount;
        if (health <= 0) {
            health = 0;
            isDead = true;
            justDied = true;
        }
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
        return newBowCoordinate.x != -999 &&
               (newBowCoordinate.equals(other->newBowCoordinate) || newBowCoordinate.equals(other->newPosition)
                || newBowCoordinate.equals(other->newSternCoordinate));
    }

    bool newBowIntersect(List<Ship *, 6> ships) {
        for (auto other : ships) {
            if (other->isDead) continue;
            if (this != other && newBowIntersect(other)) {
                return true;
            }
        }
        return false;
    }

    bool newPositionsIntersect(Ship *other) {
        bool sternCollision = newSternCoordinate.x != -999 && (newSternCoordinate.equals(other->newBowCoordinate)
                                                               || newSternCoordinate.equals(other->newPosition) ||
                                                               newSternCoordinate.equals(other->newSternCoordinate));
        bool centerCollision = newPosition.x != -999 &&
                               (newPosition.equals(other->newBowCoordinate) || newPosition.equals(other->newPosition)
                                || newPosition.equals(other->newSternCoordinate));
        return newBowIntersect(other) || sternCollision || centerCollision;
    }

    bool newPositionsIntersect(List<Ship *, 6> ships) {
        for (auto other : ships) {
            if (other->isDead) continue;
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

    void mine() {
        this->action = Action::PUTMINE;
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
                cout << "FASTER I’m a mighty pirate!" << endl;
                break;
            case Action::SLOWER:
                cout << "SLOWER a Three-Headed Monkey!" << endl;
                break;
            case Action::STARBOARD:
                cout << "STARBOARD Look behind you" << endl;
                break;
            case Action::PORT:
                cout << "PORT I wanna be a pirate!" << endl;
                break;
            case Action::FIRE:
                cout << "FIRE " << this->targetX << " " << this->targetY << endl;
                cannonCooldown = 2;
                break;
            case Action::MOVE:
                cout << "MOVE " << this->targetX << " " << this->targetY << endl;
                break;
            case Action::WAIT:
                cout << "WAIT NOOOOOOOOOOOO!" << endl;
                break;
            case Action::PUTMINE:
                cout << "MINE Neat!" << endl;
                mineCooldown = 5;
                break;
        }
    }
};

//================================================================================
// RumBarrel
//================================================================================
class RumBarrel : public Entity {
public:
    int health;

    RumBarrel() : Entity() {
        type = BARREL;
        health = -999;
    }

    RumBarrel(int id, int x, int y, int health) : Entity(BARREL, id, x, y) {
        this->health = health;
    }
};

//================================================================================
// Mine
//================================================================================
class Mine : public Entity {
public:
    Mine() : Entity() {
        type = MINE;
    }

    Mine(int id, int x, int y) : Entity(MINE, id, x, y) {
    }
};

//================================================================================
// CannonBall
//================================================================================
class CannonBall : public Entity {
public:
    CannonBall() : Entity() {
        type = CANNONBALL;
        ownerEntityId = -1;
        remainingTurns = 999;
    }

    int ownerEntityId;
    int remainingTurns;

    CannonBall(int id, int x, int y, int ownerEntityId, int remainingTurns) : Entity(CANNONBALL, id, x, y) {
        this->ownerEntityId = ownerEntityId;
        this->remainingTurns = remainingTurns;
    }
};

//================================================================================
// GameState
//================================================================================
class GameState {
public:
    List<RumBarrel, MAX_RUM_BARRELS> rumBarrels;

    List<Ship *, 3> allyShips;
    List<Ship *, 3> enemyShips;
    List<Ship *, 6> ships;

    List<Mine, 50> mines;

    List<CannonBall, 100> cannonBalls;

    List<Coord, 100> cannonBallExplosions;

    int turn;

    GameState() {
        turn = 0;
    }

    GameState(GameState const &state) {
        this->rumBarrels.count = state.rumBarrels.count;
        this->allyShips.count = state.allyShips.count;
        this->enemyShips.count = state.enemyShips.count;
        this->mines.count = state.mines.count;
        this->cannonBalls.count = state.cannonBalls.count;
        this->ships.count = 0;

        for (int i = 0; i < rumBarrels.count; ++i) {
            this->rumBarrels.array[i] = state.rumBarrels.array[i];
        }

        for (int i = 0; i < allyShips.count; ++i) {
            Ship *clonedShip = new Ship(*state.allyShips.array[i]);
            this->allyShips.array[i] = clonedShip;
            this->ships.add(clonedShip);
        }
        for (int i = 0; i < enemyShips.count; ++i) {
            Ship *clonedShip = new Ship(*state.enemyShips.array[i]);
            this->enemyShips.array[i] = clonedShip;
            this->ships.add(clonedShip);
        }

        for (int i = 0; i < mines.count; ++i) {
            this->mines.array[i] = state.mines.array[i];
        }
        for (int i = 0; i < cannonBalls.count; ++i) {
            this->cannonBalls.array[i] = state.cannonBalls.array[i];
        }
    }

    ~GameState() {
        for (int i = 0; i < ships.count; ++i) {
            delete this->ships.array[i];
        }
    }

    void moveCannonballs() {
        cannonBallExplosions.clear();
        for (int i = 0; i < cannonBalls.count; ++i) {
            CannonBall ball = cannonBalls.array[i];

            if (ball.remainingTurns == 0) {
                cannonBalls.removeAt(i);
                --i;
                continue;
            } else if (ball.remainingTurns > 0) {
                ball.remainingTurns--;
            }

            if (ball.remainingTurns == 0) {
                cannonBallExplosions.add(ball.position);
            }
        }
    }

    void decrementRum() {
        for (auto ship : ships) {
            ship->damage(1);
        }
    }

    void applyActions() {
        for (auto ship : this->ships) {
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
                        cannonBalls.array[cannonBalls.count].id = 9999;
                        cannonBalls.array[cannonBalls.count].position.x = target.x;
                        cannonBalls.array[cannonBalls.count].position.y = target.y;
                        cannonBalls.array[cannonBalls.count].ownerEntityId = ship->id;
                        cannonBalls.array[cannonBalls.count].remainingTurns = travelTime;
                        ++cannonBalls.count;
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
            }
            checkCollisions();
        }
    }

    void checkCollisions() {
        for (auto ship : ships) {
            if (ship->isDead) continue;

            Coord bow = ship->bow();
            Coord stern = ship->stern();
            Coord center = ship->position;

            // Collision with the barrels
            for (int i = 0; i < rumBarrels.count; ++i) {
                RumBarrel barrel = rumBarrels.array[i];
                if (barrel.position.equals(bow) || barrel.position.equals(stern) || barrel.position.equals(center)) {
                    ship->heal(barrel.health);

                    rumBarrels.removeAt(i);
                    --i;
                }
            }

            // Collision with the mines
            for (int i = 0; i < mines.count; ++i) {
                Mine mine = mines.array[i];
                if (mine.position.equals(bow) || mine.position.equals(stern) || mine.position.equals(center)) {
                    ship->damage(MINE_DAMAGE);

                    for (auto other : ships) {
                        if (other->isDead) continue;
                        if (other == ship) continue;

                        if (mine.position.distanceTo(ship->position) == 1 ||
                            mine.position.distanceTo(ship->stern()) == 1 ||
                            mine.position.distanceTo(ship->bow()) == 1) {
                            ship->damage(NEAR_MINE_DAMAGE);
                        }
                    }

                    mines.removeAt(i);
                    --i;
                }
            }
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
        }
        checkCollisions();
    }

    void explodeShips() {
        for (int i = 0; i < cannonBallExplosions.count; ++i) {
            Coord position = cannonBallExplosions.array[i];

            for (auto ship : ships) {
                if (ship->isDead) continue;

                if (position.equals(ship->bow()) || position.equals(ship->stern())) {
                    ship->damage(LOW_DAMAGE);
                    cannonBallExplosions.removeAt(i);
                    --i;
                    break;
                } else if (position.equals(ship->position)) {
                    ship->damage(HIGH_DAMAGE);
                    cannonBallExplosions.removeAt(i);
                    --i;
                    break;
                }
            }
        }
    }

    void explodeMines() {
        for (int i = 0; i < cannonBallExplosions.count; ++i) {
            Coord ball = cannonBallExplosions.array[i];
            for (int j = 0; j < mines.count; ++j) {
                Mine mine = mines.array[j];
                if (mine.position.equals(ball)) {
                    cannonBallExplosions.removeAt(i);
                    mines.removeAt(j);
                    --i;

                    for (auto ship : ships) {
                        if (ship->isDead) continue;

                        if (mine.position.distanceTo(ship->position) == 1 ||
                            mine.position.distanceTo(ship->stern()) == 1 ||
                            mine.position.distanceTo(ship->bow()) == 1) {
                            ship->damage(NEAR_MINE_DAMAGE);
                        }
                    }
                    break;
                }
            }
        }
    }

    void explodeBarrels() {
        for (int i = 0; i < cannonBallExplosions.count; ++i) {
            Coord ball = cannonBallExplosions.array[i];
            for (int j = 0; j < rumBarrels.count; ++j) {
                RumBarrel barrel = rumBarrels.array[j];
                if (barrel.position.equals(ball)) {
                    cannonBallExplosions.removeAt(i);
                    rumBarrels.removeAt(j);
                    --i;
                    break;
                }
            }
        }
    }


    void updateInitialRum() {
        for (auto ship : ships) {
            ship->initialHealth = ship->health;
        }
    }

    void createDroppedRum() {
        for (auto ship : ships) {
            if (ship->health <= 0 && ship->justDied) {
                int reward = min(REWARD_RUM_BARREL_VALUE, ship->initialHealth);
                if (reward > 0) {
                    rumBarrels.array[rumBarrels.count].id = 987;
                    rumBarrels.array[rumBarrels.count].position.x = ship->position.x;
                    rumBarrels.array[rumBarrels.count].position.y = ship->position.y;
                    rumBarrels.array[rumBarrels.count].health = reward;
                    ++rumBarrels.count;
                }
            }
            ship->justDied = false;
        }
    }

    void simulateTurn() {
        this->updateInitialRum();
        this->computeEnemiesActions();
        this->moveCannonballs();
        this->decrementRum();
        this->applyActions();
        this->moveShips();
        this->rotateShips();
        this->explodeShips();
        this->explodeMines();
        this->explodeBarrels();
        this->createDroppedRum();
        ++turn;
    }

    void clearLists() {
        rumBarrels.clear();
        mines.clear();
        cannonBalls.clear();
    }

    high_resolution_clock::time_point parseInputs() {
        int myShipCount; // the number of remaining ships
        cin >> myShipCount;
        cin.ignore();

        clearLists();
        for (auto ship : ships) {
            ship->isDead = true;
        }

        auto time = high_resolution_clock::now();

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
                List<Ship *, 3> *shipList = &allyShips;
                if (!ship->isAlly()) {
                    shipList = &enemyShips;
                }
                auto oldShip = (Ship *) Entity::findById((Entity **) shipList->array, shipList->count, entityId);
                if (oldShip == nullptr) {
                    shipList->add(ship);
                    ships.add(ship);
                } else {
                    oldShip->update(*ship);
                }
            } else if (entityType == "BARREL") {
                rumBarrels.array[rumBarrels.count].id = entityId;
                rumBarrels.array[rumBarrels.count].position.x = x;
                rumBarrels.array[rumBarrels.count].position.y = y;
                rumBarrels.array[rumBarrels.count].health = arg1;
                ++rumBarrels.count;
            } else if (entityType == "MINE") {
                mines.array[mines.count].id = entityId;
                mines.array[mines.count].position.x = x;
                mines.array[mines.count].position.y = y;
                ++mines.count;
            } else if (entityType == "CANNONBALL") {
                cannonBalls.array[cannonBalls.count].id = entityId;
                cannonBalls.array[cannonBalls.count].position.x = x;
                cannonBalls.array[cannonBalls.count].position.y = y;
                cannonBalls.array[cannonBalls.count].ownerEntityId = arg1;
                cannonBalls.array[cannonBalls.count].remainingTurns = arg2;
                ++cannonBalls.count;
            }
        }

        return time;
    }

    void decrementCooldown() {
        for (auto ship : ships) {
            ship->decrementCooldown();
        }
    }

    void computeRandomActions() {
        for (auto ship : allyShips) {
            if (ship->isDead) continue;
            this->computeRandomMove(ship);
        }
    }

    void computeEnemiesActions() {
        for (auto ship : enemyShips) {
            if (ship->isDead) continue;
            ship->action = WAIT;
        }
    }

    void computeRandomMove(Ship *ship) {
        ship->action = static_cast<Action>(rand() % FIRE);
    }

    void replaceActions() {
        for(auto ship : allyShips) {
            if(ship->isDead) continue;
            if(ship->action != WAIT && (ship->speed != 0 || ship->action != SLOWER)) continue;

            if(!computeMine(ship)) {
                computeFire(ship);
            }
        }
    }

    bool computeMine(Ship *ship) {
        if (ship->isMineOnCd()) {
            return false;
        }

        Coord target = ship->stern().neighbor((ship->orientation + 3) % 6);

        if(!target.isInsideMap()) return false;

        for(auto other : ships) {
            if (other->isDead) continue;
            if(other == ship) continue;

            if(target.equals(other->position) || target.equals(other->stern()) || target.equals(other->bow())) {
                return false;
            }
        }
        for(auto barrel : rumBarrels) {
            if(target.equals(barrel.position)) {
                return false;
            }
        }
        for(auto mine : mines) {
            if(target.equals(mine.position)) {
                return false;
            }
        }



        List<Ship *, 3> shipsList = allyShips;
        if (ship->isAlly()) {
            shipsList = enemyShips;
        }
        for (auto enemyShip : shipsList) {
            if (enemyShip->isDead) continue;

            const int nbTurn = 3;
            for (int i = 1; i <= nbTurn; ++i) {
                Coord centralPos = enemyShip->getPosition().neighbor(enemyShip->orientation, i * enemyShip->speed);
                Coord sternPos = centralPos.neighbor((enemyShip->orientation + 3) % 6);
                Coord bowPos = centralPos.neighbor(enemyShip->orientation);

                Coord coords[3] = {centralPos, sternPos, bowPos};
                for (int j = 0; j < 3; ++j) {
                    Coord coord = coords[j];
                    if(coord.equals(target)) {
                        ship->mine();
                        return true;
                    }
                }
            }

        }
        return false;
    }

    bool computeFire(Ship *ship) {
        if (ship->isCannonOnCd()) {
            return false;
        }

        int closestDist = 999;
        Coord closestCoord;
        List<Ship *, 3> shipsList = allyShips;
        if (ship->isAlly()) {
            shipsList = enemyShips;
        }
        for (auto enemyShip : shipsList) {
            if (enemyShip->isDead) continue;

            Coord targetCoord;
            int targetDist = 10000;
            if (enemyShip->speed == 0) {
                Coord coords[3] = {enemyShip->getPosition(), enemyShip->stern(), enemyShip->bow()};
                for (int i = 0; i < 3; ++i) {
                    Coord coord = coords[i];
                    int dist = ship->bow().distanceTo(coord);
                    int travelTime = (int) (1 + round(dist / 3.0));
                    if(travelTime < 4) {
                        targetDist = dist;
                        targetCoord = coord;
                        break;
                    }
                }
            } else {
                const int nbTurn = 4;
                for (int i = 1; i <= nbTurn; ++i) {
                    Coord centralPos = enemyShip->getPosition().neighbor(enemyShip->orientation, i * enemyShip->speed);
                    Coord sternPos = centralPos.neighbor((enemyShip->orientation + 3) % 6);
                    Coord bowPos = centralPos.neighbor(enemyShip->orientation);

                    Coord coords[3] = {centralPos, sternPos, bowPos};
                    for (int j = 0; j < 3; ++j) {
                        Coord coord = coords[j];
                        if (!coord.isInsideMap()) break;
                        int distance = ship->bow().distanceTo(coord);
                        int travelTime = (int) (1 + round(distance / 3.0));
                        if (travelTime / enemyShip->speed == i) {
                            targetDist = distance;
                            targetCoord = coord;
                            break;
                        }
                    }
                }
            }


            if (closestDist > targetDist) {
                closestCoord = targetCoord;
                closestDist = targetDist;
            }
        }
        if (closestDist > 10) {
            return false;
        }

        if (closestCoord.isInsideMap()) {
            ship->fire(closestCoord.getX(), closestCoord.getY());
            return true;
        }


        return false;
    }

//    void computeMove(Ship *ship) {
//        int barrelDist = 999;
//        Entity *closestBarrel = ship->getClosestEntity((Entity **) rumBarrels.array, rumBarrels.count,
//                                                       &barrelDist);
//        if (closestBarrel == nullptr) {
//            int enemyDist = 999;
//            Ship *closestEnemy = (Ship *) Entity::getClosestEntity((Entity **) enemyShips.array,
//                                                                   enemyShips.count,
//                                                                   ship->bow(),
//                                                                   &enemyDist);
//            ship->move(closestEnemy->getPosition().x, closestEnemy->getPosition().y);
//        } else {
//            Coord shipCoord = ship->position;
//            Coord fasterCoord = shipCoord.neighbor(ship->orientation, ship->speed + 1);
//            Coord portCoord = shipCoord.neighbor(ship->orientation, ship->speed).neighbor(
//                    (ship->orientation + 1) % 6);
//            Coord starboardCoord = shipCoord.neighbor(ship->orientation, ship->speed).neighbor(
//                    (ship->orientation == 0 ? 5 : ship->orientation - 1));
//
//            int portDist = portCoord.distanceTo(closestBarrel->getPosition());
//            int starDist = starboardCoord.distanceTo(closestBarrel->getPosition());
//            int fastDist = fasterCoord.distanceTo(closestBarrel->getPosition());
//
//
//            if (fasterCoord.isInsideMap() && fastDist <= starDist && fastDist <= portDist) {
//                ship->faster();
//            } else if ((ship->speed > 0 || barrelDist == 1) && portCoord.isInsideMap() &&
//                       portDist <= starDist && portDist <= fastDist) {
//                ship->port();
//            } else if ((ship->speed > 0 || barrelDist == 1) && starboardCoord.isInsideMap() &&
//                       starDist <= fastDist && starDist <= portDist) {
//                ship->starboard();
//            } else {
//                ship->move(closestBarrel->getPosition().getX(), closestBarrel->getPosition().getY());
//            }
//
//        }
//    }

    int eval() {
        int score = 0;
        for (auto ship : allyShips) {
            if (ship->isDead) continue;
            score += ship->health;

            for (int i = 0; i < rumBarrels.count; ++i) {
                RumBarrel barrel = rumBarrels.array[i];
                int dist = ship->distanceTo(barrel);
                score -= dist / 10;

            }

            score += ship->speed;
            if (ship->speed == 0) {
                score -= 2;
            }
        }

        return score;
    }

    void sendOutputs() {
        ++turn;
        for (auto ship : allyShips) {
            if (ship->isDead) continue;

            ship->sendOutput();
        }
    }
};

GameState *full_random_strategy(GameState *state, high_resolution_clock::time_point start) {
    GameState *bestState = new GameState(*state);
    int bestScore = -99999;
    for (int j = 0; j < 100000; ++j) {
        GameState *baseState = new GameState(*state);
        baseState->computeRandomActions();

        GameState *endState = new GameState(*baseState);

        endState->simulateTurn();
        int score = endState->eval();

        for (int i = 0; i < DEPTH; ++i) {
            endState->computeRandomActions();
            endState->simulateTurn();
            score += endState->eval();
        }
        delete endState;

        if (score > bestScore) {
            delete bestState;
            bestState = baseState;
            bestScore = score;
        } else {
            delete baseState;
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start).count();
        if (duration > 40) {
//            cerr << "iteration:" << j << endl;
            delete state;
            return bestState;
        }
    }
    delete state;

    return bestState;
}

//================================================================================
// Main
//================================================================================
int main() {

    GameState *state = new GameState();

    while (1) {

        auto start = state->parseInputs();

        state->decrementCooldown();

        state = full_random_strategy(state, start);

        state->replaceActions();

        state->sendOutputs();

    }
}

#pragma clang diagnostic pop
