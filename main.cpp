#include <iostream>
#include <unistd.h>
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

        // test speed for optimization (debugging)
        int averageSpeed = 0;
        int maxTime = 0;
        int counter = 1;

        // Game information
        const double gameSpeed = 16.6; // How many miliseccond per refresh
        const int width = 11;
        const int height = 22;
        const int mapSize = width*height;
        const int air = 0;
        int option = 0;
        const char *color[8] ={GREY,CYAN,BLUE,WHITE,YELLOW,GREEN,MAGNETA,RED};

        int x,y,cord,blockId,totalRotationStep;
        int defaultSpeed,speed,level,nextLevelHandler,movingCycle,sensitivity;
        const int maxLevel = 16;

        unsigned int score = 0;
        int nextBlockId;

        // For optimization
        int collisionYCordinate = height; // Calculate shortest step before collision

        // Get user input;
        unsigned int rotationStep; // số thực không âm (real number)
        int toggleTimer; // equal to sensitivity left and right button
        bool keyPressed2 = false; // Q and E button (rotate left and right) (To prevent toggle and 2 keys press at the same time)
        bool keyPressed3 = false; // down and space button

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

    public:
        GameTetris(){ // Constructor
            toggleTimer = 0;
            movingCycle = 50/gameSpeed; // ~ 30 miliseccond; = 3 refresh time per 25 milisec game speed
            // Dynamic array allocate on the heap
            map = new int[mapSize]();
            renderingMap = new int[mapSize]();
            renewMap();
        }
        int blockData(int col,int row,int id);
        // string coloring(int blockIdIndex);
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
        case 1: return iBlock[temp];
        case 2: return jBlock[temp];
        case 3: return lBlock[temp];
        case 4: return oBlock[temp];
        case 5: return sBlock[temp];
        case 6: return tBlock[temp];
        case 7: return zBlock[temp];
    }
    return 0;
}

void GameTetris::renewMap(){ // temporary
    x = 3;
    y = 0;
    cord = index(x,y);
    score = 0;
    level = 1;
    level--;
    nextLevelHandler = 0;

    srand(time(NULL));
    nextBlockId = rand()%7 + 1;

    for (int i = 0; i < mapSize; i++){
        map[i] = air;
    }
    intializer();
}

void GameTetris::calculateCollisionStep(){
    // check colision code 2 (For optimization: reduce time cycle (calculate step before colision))
    int minCollisionStep = height;
    for (int i = 0; i < 4; i++){ // Collision detection (moving down)
        int collisionStep = 0;
        int temp = index(x,y+1) + block[i];
        for (temp; temp < index(0,height); temp+= width){
            if (map[temp] != air){break;}
            collisionStep++;
        }
        if (collisionStep < minCollisionStep){minCollisionStep = collisionStep;}
    }
    collisionYCordinate = y + minCollisionStep;
}

void GameTetris::intializer(){
    if (score >= 100*nextLevelHandler && level < (maxLevel - 1)){
        nextLevelHandler++;
        level++;
        defaultSpeed = 2*(maxLevel - level)-1;
        speed = defaultSpeed;

        // Compare speed and moving cycles of each level4
        int checkLevelSensitivity = 2*level/((2*maxLevel) - movingCycle);// return 1 or 0; !checkcondition = -checkCondition+1
        sensitivity = checkLevelSensitivity*defaultSpeed + (-checkLevelSensitivity+1)*movingCycle;
    }


    blockId = nextBlockId;
    //srand(time(NULL)); // time as random seed 
    nextBlockId = rand()%7 + 1;
    rotationStep = 0;
    switch (blockId) {
        case 1: totalRotationStep = (sizeof(iBlock)/sizeof(iBlock[0]))/4;break;
        case 2: totalRotationStep = (sizeof(jBlock)/sizeof(jBlock[0]))/4;break;
        case 3: totalRotationStep = (sizeof(lBlock)/sizeof(lBlock[0]))/4;break;
        case 4: totalRotationStep = (sizeof(oBlock)/sizeof(oBlock[0]))/4;break;
        case 5: totalRotationStep = (sizeof(sBlock)/sizeof(sBlock[0]))/4;break;
        case 6: totalRotationStep = (sizeof(tBlock)/sizeof(tBlock[0]))/4;break;
        case 7: totalRotationStep = (sizeof(zBlock)/sizeof(zBlock[0]))/4;break;
    }
    for (int i = 0;i < sizeof(block)/sizeof(block[0]);i++){
        block[i] = blockData(i,rotationStep%totalRotationStep,blockId);
    }
    calculateCollisionStep();
    updateUiLayout();
}

void GameTetris::updateUiLayout(){
    cls();
    //char *layout = "adasda";

    string layout = "";
    string space = string(width*2,' ');
    string nextPiecesField = "";
    string statusField;

    int temp = 0;
    for (int i = 0; i < 4; i++){
        nextPiecesField += space+" #  |";
        for (int j = 0;j < 4;j++){
            if (index(j,i) != blockData(temp,0,nextBlockId)){
                nextPiecesField += WHITE" |";
            } else {
                nextPiecesField += color[nextBlockId];
                nextPiecesField += "O|";
                temp++;
            }
        }
        nextPiecesField += WHITE"\n";
    }
    statusField = 
        space+" #*******************\n"+
        space+" #      IN-GAME STATUS\n"+
        space+" # - Level: "+to_string(level)+"\n"+
        space+" # - Score: "+to_string(score)+"\n"+
        space+" # - Speed: "+to_string(defaultSpeed)+"\n"+
        space+" # - Total Level: "+to_string(maxLevel - 1)+"\n"+
        space+" # - Next piece ID: "+to_string(nextBlockId)+" \n"+
        space+" #*******************\n"+
        space+" #     DEBUG MENU\n"+
        space+" # - Frame rate (ms): "+to_string(gameSpeed)+" ms ("+to_string(1000/gameSpeed)+" fps)\n"+
        space+" # - Current sensitivity: "+to_string(sensitivity)+"\n"+
        space+" # - Default sensitivity: "+to_string(movingCycle)+"\n"+
        space+" # - Avg.Time: "+to_string(averageSpeed/counter)+" ms  \n"+
        space+" # - Max.Time: "+to_string(maxTime)+" ms  \n";

    layout = space+WHITE+" #*******************\n"+space+" #     NEXT\n"+nextPiecesField+space+" #             \n" + statusField;

    averageSpeed = 0;
    maxTime = 0;
    counter = 1;

    printf("%s\n",layout.c_str());
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

        
        //if (coloringId != renderingMap[k]){
        //    gameField = gameField+color[renderingMap[k]];
        //}

        if (renderingMap[k] != air){
            gameField = gameField +color[renderingMap[k]]+ "O|";
        } else {
            gameField = gameField +color[renderingMap[k]]+" |";
        }
        //Ui = Ui + to_string(renderingMap[k])+"|";
    }
    printf("%s\n",gameField.c_str());
}

void GameTetris::controller(){

    // Move left and right
    if (GetAsyncKeyState(VK_LEFT) != GetAsyncKeyState(VK_RIGHT) && toggleTimer >= sensitivity){
        toggleTimer = 0;
        if (GetAsyncKeyState(VK_LEFT) < 0){
            for (int i = 0; i < 4; i++){ // Collision detection (moving left)
                int temp = index(x-1,y) + block[i];
                if (map[temp] != air || temp % width == 0){break;}
                if (i == 3){x--;};
            }
        } else if (GetAsyncKeyState(VK_RIGHT) < 0){
            for (int j = 0; j < 4; j++){ // Collision detection (moving right)
                int temp = index(x+1,y) + block[j];
                if (map[temp] != air || temp % width == 0){break;}
                if (j == 3){x++;}
            }
        }
        calculateCollisionStep();
    } else if (toggleTimer < sensitivity){
        toggleTimer++;
    }
    /*

    // shitty code down here
    if (leftKeyPressed == rightKeyPressed){
        toggleTimer = -(sensitivity*(1/testVariable));
        if (GetAsyncKeyState(VK_LEFT) < 0){ leftKeyPressed = true;testVariable++;
        } else if (GetAsyncKeyState(VK_RIGHT) < 0){ rightKeyPressed = true;testVariable++;} else {
            testVariable=1;
        }
    } else if (leftKeyPressed != rightKeyPressed){
        toggleTimer++;
        if (toggleTimer == sensitivity){
            if (leftKeyPressed){
                for (int i = 0; i < 4; i++){ // Collision detection (moving left)
                    int temp = index(x-1,y) + block[i];
                    if (map[temp] != -1 || temp % width == 0){break;}
                    if (i == 3){x--;};
                }
            } else if (rightKeyPressed){
                for (int j = 0; j < 4; j++){ // Collision detection (moving right)
                    int temp = index(x+1,y) + block[j];
                    if (map[temp] != -1 || temp % width == 0){break;}
                    if (j == 3){x++;}
                }
            }
            leftKeyPressed = false;
            rightKeyPressed = false;
            calculateCollisionStep();
        }
    }*/

    // Rotate left and rotate right
    if ((GetAsyncKeyState(0x51) == GetAsyncKeyState(0x45)) && keyPressed2){
        keyPressed2 = false;
    } else if ((GetAsyncKeyState(0x51) != GetAsyncKeyState(0x45)) && !keyPressed2){
        keyPressed2 = true;
        if (GetAsyncKeyState(0x51) < 0){ // Left Rotation
            for (int i = 0; i < 4; i++){ // Collision detection (rotate left)
                int temp = index(x,y) + blockData(i,(rotationStep-1)%totalRotationStep,blockId);
                if (map[temp] != air || temp % width == 0 || temp > index(0,height)){break;}
                if (i == 3){rotationStep--;};
            }
        } else if (GetAsyncKeyState(0x45) < 0){ // Right Rotation
            for (int j = 0; j < 4; j++){ // Collision detection (rotate right)
                int temp = index(x,y) + blockData(j,(rotationStep+1)%totalRotationStep,blockId);
                if (map[temp] != air || temp % width == 0 || temp > index(0,height)){break;}
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
        speed = defaultSpeed;
        keyPressed3 = false;
    } else if ((GetAsyncKeyState(VK_DOWN) != GetAsyncKeyState(VK_SPACE)) && !keyPressed3){
        keyPressed3 = true;
        if (GetAsyncKeyState(VK_DOWN) < 0){speed = 1;
        } else if (GetAsyncKeyState(VK_SPACE) < 0){y = collisionYCordinate;speed = 1;}
        
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
                if (map[index(col,row)] != air){
                    totalBlockPerRow++;
                }
            }
            if (totalBlockPerRow != 10){
                for (int k = 0; k < width; k++){
                    map[index(k,setToMapRow)] = map[index(k,row)];
                }
                setToMapRow--;
                continue;
            }
            score += 10;
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
        if (i >= speed){
            movingDown();
        } else {
            controller();
        }
        cord = index(x,y);
        updateConsoleGraphics();

        clock_t end = clock();
        averageSpeed += end - start;
        counter++;
        
        if (averageSpeed/counter > maxTime){ maxTime = averageSpeed/counter;};
        //cout << end - start << + " " << endl;
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
    /*
    #if defined(NOSYNC)
        std::cout.sync_with_stdio(false);
    #endif*/

    
    ios_base::sync_with_stdio(false);
    GameTetris box;
    box.run();
    /*
    system("color 0F");
    // compare speed
    char *test = "Mamamia";
    clock_t start = clock();

    for (int i = 0; i < 1000; i++){
        //write(1, test , strlen(test));
        //cout << "Mamamia";
        //write(2,test,strlen(test));
    }
    clock_t end = clock();
    cout << end - start << + " " << endl;
    */



    system("pause");
    return 0;
}