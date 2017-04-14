#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/
int main()
{

    // game loop
    while (1) {
        int myShipCount; // the number of remaining ships
        cin >> myShipCount; cin.ignore();
        int entityCount; // the number of entities (e.g. ships, mines or cannonballs)
        cin >> entityCount; cin.ignore();
        for (int i = 0; i < entityCount; i++) {
            int entityId;
            string entityType;
            int x;
            int y;
            int arg1;
            int arg2;
            int arg3;
            int arg4;
            cin >> entityId >> entityType >> x >> y >> arg1 >> arg2 >> arg3 >> arg4; cin.ignore();
        }
        for (int i = 0; i < myShipCount; i++) {

            // Write an action using cout. DON'T FORGET THE "<< endl"
            // To debug: cerr << "Debug messages..." << endl;

            cout << "MOVE 11 10" << endl; // Any valid action, such as "WAIT" or "MOVE x y"
        }
    }
}
