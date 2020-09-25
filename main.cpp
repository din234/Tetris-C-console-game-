#include <iostream>
#include <windows.h>
#include <ctime>

using namespace std;

#define RED "\x1B[31m"
#define GREEN "\x1B[32m"
#define YELLOW "\x1B[33m"
#define BLUE "\x1B[34m"
#define MAGNETA "\x1B[35m"
#define WHITE "\x1B[37m"
#define CYAN "\x1B[36m"
#define GREY "\x1B[90m"

class GameTetris
{
    private:
        // biến 1D array thành 2D array: 1DArrayIndex = x + y*width;
        inline int index(int posx,int posy) {return posx + (posy * width);};

        // Game information
        const int gameSpeed = 17; // How many miliseccond per refresh
        const int width = 11;
        const int height = 22;
        const int mapSize = width*height;
        int option = 0;
        int x,y,cord,speed,blockId,totalRotationStep;
        unsigned int score = 0;
        int nextBlockId;

        // For optimization
        int collisionYCordinate = height; // Calculate shortest step before collision

        // Get user input
        unsigned int rotationStep; // số thực không âm (real number)
        bool keyPressed1; // Left and right arrow (To prevent toggle and 2 keys press at the same time)
        bool keyPressed2; // Q and E button (rotate left and right)
        bool keyPressed3; // down and space button

        // memory on heap
        int* map;
        int* renderingMap;
        
        // Static array allocate memory on stack (allocate memory on stack is faster than on heap)
        int iBlock[8] { // long I: id=0
            index(0,2),index(1,2),index(2,2),index(3,2),
            index(2,0),index(2,1),index(2,2),index(2,3)
        };
        int jBlock[16] { // Reverse-L Block: id=1
            index(1,1),index(1,2),index(2,2),index(3,2),
            index(2,1),index(3,1),index(2,2),index(2,3),
            index(1,2),index(2,2),index(3,2),index(3,3),
            index(2,1),index(2,2),index(1,3),index(2,3)
        };
        int lBlock[16] { // L Block: id=2
            index(3,1),index(1,2),index(2,2),index(3,2),
            index(2,1),index(2,2),index(2,3),index(3,3),
            index(1,2),index(2,2),index(3,2),index(1,3),
            index(1,1),index(2,1),index(2,2),index(2,3)
        };
        int oBlock[4] { // Square Block: id=3
            index(1,1),index(2,1),index(1,2),index(2,2)
        };
        int sBlock[8] { // S Block: id=4
            index(2,1),index(3,1),index(1,2),index(2,2),
            index(2,1),index(2,2),index(3,2),index(3,3)
        };
        int tBlock[16] { // T Block: id=5
            index(2,1),index(1,2),index(2,2),index(3,2),
            index(2,1),index(2,2),index(3,2),index(2,3),
            index(1,2),index(2,2),index(3,2),index(2,3),
            index(2,1),index(1,2),index(2,2),index(2,3)
        };
        int zBlock[8] { // Z Block: id=6
            index(1,1),index(2,1),index(2,2),index(3,2),
            index(3,1),index(2,2),index(3,2),index(2,3)
        };

        int block[4] = {}; // intialize array

        // int pieceSlot[2] = {}; // for next piece

    public:
        GameTetris(){ // Constructor
            speed = 5;
            nextBlockId = rand()%7;
            // Dynamic array allocate on the heap
            map = new int[mapSize]();
            renderingMap = new int[mapSize]();
            renewMap();
        }
        int blockData(int col,int row,int id);
        string coloring(int blockIdIndex);
        void calculateCollisionStep();

        void renewMap(); // temporary
        void intializer();
        
        void updateUiLayout();
        void updateConsoleGraphics();

        void controller();
        void movingDown();
        void updateLogic();
        int run();
        friend void cls();
};

void cls()
{
    HANDLE hOut;
    COORD Position;

    hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    Position.X = 0;
    Position.Y = 0;
    SetConsoleCursorPosition(hOut, Position);
}

int GameTetris::blockData(int col,int row,int id){
    int temp = col + row*4;
    switch (id) {
        case 0: return iBlock[temp];
        case 1: return jBlock[temp];
        case 2: return lBlock[temp];
        case 3: return oBlock[temp];
        case 4: return sBlock[temp];
        case 5: return tBlock[temp];
        case 6: return zBlock[temp];
    }
    return 0;
}

string GameTetris::coloring(int blockIdIndex){
    switch (blockIdIndex){
        case(-1):return GREY;
        case(0):return CYAN;
        case(1):return BLUE;
        case(2):return WHITE;
        case(3):return YELLOW;
        case(4):return GREEN;
        case(5):return MAGNETA;
        case(6):return RED;
    }
    return "";
}

void GameTetris::calculateCollisionStep(){
    // check colision code 2 (For optimization: reduce time cycle (calculate step before colision))
    int minCollisionStep = height;
    for (int i = 0; i < 4; i++){ // Collision detection (moving down)
        int collisionStep = 0;
        int temp = index(x,y+1) + block[i];
        for (temp; temp < index(0,height); temp+= width){
            if (map[temp] != -1){break;}
            collisionStep++;
        }
        if (collisionStep < minCollisionStep){minCollisionStep = collisionStep;}
    }
    collisionYCordinate = y + minCollisionStep;
}

void GameTetris::renewMap(){ // temporary
    x = 3;
    y = 0;
    cord = index(x,y);
    score = 0;
    for (int i = 0; i < mapSize; i++){
        map[i] = -1;
    }
    intializer();
}

void GameTetris::intializer(){
    blockId = nextBlockId;
    nextBlockId = rand()%7;
    rotationStep = 0;
    switch (blockId) {
        case 0: totalRotationStep = (sizeof(iBlock)/sizeof(iBlock[0]))/4;break;
        case 1: totalRotationStep = (sizeof(jBlock)/sizeof(jBlock[0]))/4;break;
        case 2: totalRotationStep = (sizeof(lBlock)/sizeof(lBlock[0]))/4;break;
        case 3: totalRotationStep = (sizeof(oBlock)/sizeof(oBlock[0]))/4;break;
        case 4: totalRotationStep = (sizeof(sBlock)/sizeof(sBlock[0]))/4;break;
        case 5: totalRotationStep = (sizeof(tBlock)/sizeof(tBlock[0]))/4;break;
        case 6: totalRotationStep = (sizeof(zBlock)/sizeof(zBlock[0]))/4;break;
    }
    for (int i = 0;i < sizeof(block)/sizeof(block[0]);i++){
        block[i] = blockData(i,rotationStep%totalRotationStep,blockId);
    }
    calculateCollisionStep();
    updateUiLayout();
}

void GameTetris::updateUiLayout(){
    cls();
    string layout = "";
    string space = string(width*2,' ');
    string nextPiecesField = "";

    int temp = 0;
    for (int i = 0; i < 4; i++){
        nextPiecesField += space+WHITE" #  |";
        for (int j = 0;j < 4;j++){
            if (index(j,i) != blockData(temp,0,nextBlockId)){
                nextPiecesField += WHITE" |";
            } else {
                nextPiecesField += coloring(nextBlockId)+"O|";
                temp++;
            }
        }
        nextPiecesField += WHITE"  #\n";
    }
        /*for (int j = 0; j < width; j++){
            layout += " |";
        }*/
    layout = space+WHITE+" #     NEXT    #\n"+nextPiecesField+space+" # ----------- #";
        cout << layout;
}

void GameTetris::updateConsoleGraphics(){
    cls();
    string gameField = ""; // Optimizing display
    int coloringId; // Prevent repetitive coloring

    for (int i = 0; i < mapSize; i++){
        renderingMap[i] = map[i];
    }
    for (int j = 0; j < 4; j++){
        if (cord + block[j] <= mapSize){
            renderingMap[cord + block[j]] = blockId;
        }
    }
    
    for (int k = width*2; k < mapSize; k++){
        if (k % width == 0){gameField = gameField + "\n";}

        if (coloringId != renderingMap[k]){
            gameField = gameField+coloring(renderingMap[k]);
        }

        if (renderingMap[k] != -1){
            gameField = gameField + "O|";
        } else {
            gameField = gameField + " |";
        }
        //Ui = Ui + to_string(renderingMap[k])+"|";
    }
    cout << gameField << "Test: " << score << " " << endl;
}

void GameTetris::controller(){
    // Move left and right
    if ((GetAsyncKeyState(VK_LEFT) == GetAsyncKeyState(VK_RIGHT)) && keyPressed1){
        keyPressed1 = false;
    } else if ((GetAsyncKeyState(VK_LEFT) != GetAsyncKeyState(VK_RIGHT)) && !keyPressed1){
        keyPressed1 = true;
        if (GetAsyncKeyState(VK_LEFT) < 0){
            for (int i = 0; i < 4; i++){ // Collision detection (moving left)
                int temp = index(x-1,y) + block[i];
                if (map[temp] != -1 || temp % width == 0){break;}
                if (i == 3){x--;};
            }
        } else if (GetAsyncKeyState(VK_RIGHT) < 0){
            for (int j = 0; j < 4; j++){ // Collision detection (moving right)
                int temp = index(x+1,y) + block[j];
                if (map[temp] != -1 || temp % width == 0){break;}
                if (j == 3){x++;}
            }
        }
        calculateCollisionStep();
    }

    // Rotate left and rotate right
    if ((GetAsyncKeyState(0x51) == GetAsyncKeyState(0x45)) && keyPressed2){
        keyPressed2 = false;
    } else if ((GetAsyncKeyState(0x51) != GetAsyncKeyState(0x45)) && !keyPressed2){
        keyPressed2 = true;
        if (GetAsyncKeyState(0x51) < 0){ // Left Rotation
            for (int i = 0; i < 4; i++){ // Collision detection (rotate left)
                int temp = index(x,y) + blockData(i,(rotationStep-1)%totalRotationStep,blockId);
                if (map[temp] != -1 || temp % width == 0 || temp > index(0,height)){break;}
                if (i == 3){rotationStep--;};
            }
        } else if (GetAsyncKeyState(0x45) < 0){ // Right Rotation
            for (int j = 0; j < 4; j++){ // Collision detection (rotate right)
                int temp = index(x,y) + blockData(j,(rotationStep+1)%totalRotationStep,blockId);
                if (map[temp] != -1 || temp % width == 0 || temp > index(0,height)){break;}
                if (j == 3){rotationStep++;};
            }
        }
        // update
        for (int k = 0;k < 4;k++){
            block[k] = blockData(k,rotationStep % totalRotationStep,blockId);
        }
        calculateCollisionStep();
    }

    if ((GetAsyncKeyState(VK_DOWN) == GetAsyncKeyState(VK_SPACE)) && keyPressed3){
        // speed = speed/4;
        keyPressed3 = false;
    } else if ((GetAsyncKeyState(VK_DOWN) != GetAsyncKeyState(VK_SPACE)) && !keyPressed3){
        //speed = speed*4;
        keyPressed3 = true;
        if (GetAsyncKeyState(VK_SPACE) < 0){y = collisionYCordinate;}
    }
}

void GameTetris::movingDown(){
    if (y == collisionYCordinate){
        int setToMapRow = height - 1;
        // check to clear (brute force code; not optimizing)
        for(int k = 0;k<4;k++){
            map[cord + block[k]] = blockId;;
        }
        for (int row = height - 1; row >= 0; row--){
            int totalBlockPerRow = 0;
            for (int col = 0; col < width; col++){
                if (map[index(col,row)] != -1){
                    totalBlockPerRow++;
                }
            }
            if (totalBlockPerRow != 10){
                for (int k = 0; k < width; k++){
                    renderingMap[index(k,setToMapRow)] = map[index(k,row)];
                }
                setToMapRow--;
                continue;
            }
            score += 10;
        }

        for (int i = 0; i < mapSize; i++){
            map[i] = renderingMap[i];
        }
        x = 3;
        y = 0;
        cord = index(x,y);
        intializer();
    } else if (y > collisionYCordinate){
        cout << "Gameover";
        system("pause");
        option=0;
    }
    y++;
}

void GameTetris::updateLogic(){
    for (int i = 0; i <= speed; i ++){
        clock_t start = clock();
        if (i == speed){
            movingDown();
        } else {
            controller();
        }
        cord = index(x,y);
        updateConsoleGraphics();

        clock_t end = clock();
        cout << end - start << + " " << endl;
        if (end - start < gameSpeed){
            Sleep(gameSpeed - (end - start));
        }
    }


    

        /*
        // check colision code 1: brute force code
        for (int i = 0; i < 4; i++){ // Collision detection (moving down)
            int temp = index(x,y+1) + block[i];
            if (map[temp] != -1 || temp > index(0,height)){
                for (int j = 0; j < 4; j++){
                    map[index(x,y) + block[j]] = blockId;
                }
                x = 3;
                y = 0;
                cord = index(x,y);
                intializer();
                break;
            }if (i == 3){y++;};
        }*/
}

int GameTetris::run(){
    while (true){
        system("cls");
        system("Color 0F");
        cout << "Choose the following option: \n1. Start game.\n2.Exist" << endl;
        cin >> option;
        switch (option){
            case 0: break;
            case 1: renewMap(); while (option == 1) {updateLogic();}break;
            case 2: return 0;
            case 3: updateUiLayout(); break;
            default: cout << "Invalid option!!!"<<endl;system("pause");option = 0;
        }
    }
    return 0;
}


int main() {
    GameTetris box;
    box.run();
    //system("pause");
    return 0;
}