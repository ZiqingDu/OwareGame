#include <iostream>
#include <iomanip>
#include <climits>
#include <tuple>
#include <sys/time.h> 
#include <thread> 
#include <unistd.h>
#include <random>
#include <cstdlib>
#ifdef _OPENMP
#include <omp.h>
#endif
using namespace std;

#define NUM_HOLES 12 // (6 regular holes ) * 2 players = 12
static volatile sig_atomic_t signal1=0;

void setTime(){
  usleep(1990000);
  signal1=1;
}

typedef struct {
	int score; //score: minmax score
	bool color;
	//int colormove;
	int specialmove;
	int hole;
  int bestScore;
}Node;

const int NULLKEY=0;
enum ScoreType { Exact=0, Alpha=1, Beta=2 };

typedef struct {
    unsigned long long int key=NULLKEY; 
    Node bestmove;
    int depth;
    ScoreType bound;
    int stale;
}hasht;

typedef struct{
    hasht *arr;
    unsigned long long int size;
}hashtable;

hashtable thash;
class OwareBoard;
void HashEntry(unsigned long long int hash, int depth, int stale, Node bestMove, ScoreType bound);
void initTable();
unsigned long long int geneKey(OwareBoard *t);
hasht FindHashTable(unsigned long long int t);
void insertHashTable(unsigned long long int t,int depth, Node bestMove, ScoreType bound);

//class to store the OwareBoard
class OwareBoard {
  public:
    int redBoard[NUM_HOLES]; // an array to store the number of red seed for each hole
    int blackBoard[NUM_HOLES];// an array to store the number of black seed for each hole
    int specialBoard[NUM_HOLES];// an array to store the number of special seed for each hole
    int scoreBoard[2];// an array to store the number of captured seed for each player
    int totalSeeds[NUM_HOLES];// an array to store the number of total seed for each hole
    int changeSeeds[NUM_HOLES];// an array to store the type of last-place seed for each hole: 1-red; 2-black; 3-special one
    int playerFirst;// to store which player need a suggestion

    OwareBoard() {
      // Initial
      scoreBoard[0]=0;
      scoreBoard[1]=0;
      for (int i = 0; i < NUM_HOLES; i++) {
          redBoard[i] = 3;
          blackBoard[i] = 3;
          specialBoard[i] = 0;
          totalSeeds[i]=6;
          changeSeeds[i] = 0;
      }
    }

    // Play a game between two playerIsFirst
    void playGame() {
      bool playerIsFirst = true; // Is it the first player's turn?
      int playerHole;
      bool goAgain;// check if this turn valid
      char playColor;
      bool color;
      int specialPlay;
      double tdiff;

      //do when not satisfy the end rules
      while (!isGameOver()) {
        printBoard();
        if (playerIsFirst) {
          if (playerFirst==1) {
            searching();//use the minmax function to find the best move
          }
          cout << "Player 1: Select the move rule (hole from 1 to 6)." << endl;
        }
        else {
          if (playerFirst==-1) {
            searching();//use the minmax function to find the best move
          }
          cout << "Player 2: Select the move rule (hole from 7 to 12)." << endl;
        }
        cin.ignore(INT_MAX, '\n'); //clear the cache
        cin>> playerHole>>playColor>>specialPlay; //input the move rule,like: 3R2
        //change the char to a bool value
        if (playColor=='R') {
          color=false;}
          else color=true;
        // Player goes again if they donot enter a valid rule
        bool isValid = checkValidMove1(playerHole,specialPlay,playerIsFirst);
        if (!isValid) 
          goAgain = true;
        else 
        //follow the inputs to update the whole board
          goAgain = updateBoard(playerHole,color,specialPlay, playerIsFirst);
        //if this turn is valid, change player 
        if (!goAgain) 
          playerIsFirst = !playerIsFirst;
        else cout << "Invalid move. Try a different hole." << endl;
      }
    }

    // Update the Oware board after a player has made a move
    bool updateBoard(int hole,bool color, int specialmove, bool playerIsFirst) {
      
      // Player goes again if they donot enter a valid rule
      bool isValid = checkValidMove2(hole,playerIsFirst);
      if (!isValid) {
        //cout << "Invalid move. Try a different hole." << endl;
        return true;
      }

      //make a copy board
      OwareBoard* tp = this->copy();

      //Sowing
      hole=sowing(hole , color, specialmove);

      int p=0;int t=0;
      if (!playerIsFirst) { t=1;}

     //capturing
     int changeSeed=changeSeeds[hole-1];
      while (1){
        hole --;
        if (playerIsFirst) {
          if (hole < 6 ) break; //check if the current hole is the hole of the opponent
          p=captureValid(hole,changeSeed);
          scoreBoard[0]=scoreBoard[0]+p;
        }else {
          if (hole >5 ||hole <0) break;
          p=captureValid(hole,changeSeed);
          scoreBoard[1]=scoreBoard[1]+p;
        }
        if (p==0) break;//if this turn cannot capture any seed, stop it
      }

      p=0;
      //check if it is starving the opponent,yes than regard it as a unvalid move 
      for (int i = 6; i < 12; i++) {
          if (totalSeeds[i-t*6] != 0) {
            p=1;break;
          }
      }
      if (p==0) {
        //cout << "Starving. Try a different hole." << endl;
        for (int i = 0; i < NUM_HOLES; i++) {
          blackBoard[i]= (tp->blackBoard[i]);
          redBoard[i]= (tp->redBoard[i]);
          blackBoard[i]= (tp->blackBoard[i]);
          specialBoard[i]= (tp->specialBoard[i]);
          totalSeeds[i]= (tp->totalSeeds[i]);
          changeSeeds[i]= (tp->changeSeeds[i]);
        }
        scoreBoard[0]= (tp->scoreBoard[0]);
        scoreBoard[1]= (tp->scoreBoard[1]);
        delete tp;
        return true;
      }
      p=1;
      //check if mine is starving 
      for (int i = 0; i < 6; i++) {
        if (totalSeeds[i+t*6] != 0) {
          p=2;break;
        }
      }
      //check: if any move can make not starving
      if (p==1) {
        for (int i = 11; i >5; i--) {
          if (totalSeeds[i-t*6]+i>(12-t*6)){ 
            p=0;break;
          }
        }
      }
      //When there is no valid move to make not starving, each player takes the seeds of its side
      if (p==1){
        for (int i = 0; i < 6; i++) {
          scoreBoard[0] +=totalSeeds[i];
          redBoard[i] = 0;
          blackBoard[i] = 0;
          specialBoard[i] = 0;
          totalSeeds[i]=0;
          scoreBoard[1] +=totalSeeds[i+6];
          redBoard[i+6] = 0;
          blackBoard[i+6] = 0;
          specialBoard[i+6] = 0;
          totalSeeds[i+6]=0;
        }
      }

      //show the current status
      //printBoard();

      delete tp;
      return false;
    }

    //Goes around the board in a loop and deposits seeds along the way
    int sowing(int hole,bool color,int specialmove){
      int ihole=hole;
      int redStones = redBoard[hole-1];
      int blackStones = blackBoard[hole-1];
      int specialStones = specialBoard[hole-1];
      int total=totalSeeds[hole-1];
      int p=0;//to show if it has spacial seed;0-no;1-yes
      if (specialStones>0) { p=1;} 

      // Remove the seeds
      redBoard[hole-1] = 0;
      blackBoard[hole-1]=0;
      specialBoard[hole-1]=0;
      totalSeeds[hole-1]=0;

      int k=0;//to show the iteration number of whole board
      int q=0;//to show the skip number for the original hole
      //deposits
      while (total > 0) {
        if (hole==12) k +=1;
        hole = hole % NUM_HOLES;//make the step change to the hole

        // Skip the hole if it's the original hole
        if (hole == (ihole-1)) {
          q +=1;
          hole = (hole + 1) % NUM_HOLES;
        }

        //special seed sowing
        //Check if the current step is equal to the assigned step
        if (p==1 && ((hole+k*(NUM_HOLES)) == (ihole+specialmove-1+q))) {
          for (int i=0;i<specialStones;i++){
            specialBoard[(hole+i)% NUM_HOLES] +=1;
            totalSeeds[(hole+i)% NUM_HOLES] +=1;
            total--;
            changeSeeds[(hole+i)% NUM_HOLES] =3;
          }
          p=0;
          hole = (hole + specialStones)% NUM_HOLES;
          if (total == 0) break; 
          if (hole == (ihole-1)) {
          q +=1;
          hole = (hole + 1) % NUM_HOLES;
        }
        }

        //red and black seed sowing
        if (color){
            if (blackStones > 0) {
              blackBoard[hole] +=1;totalSeeds[hole] +=1;blackStones --;changeSeeds[hole]=2;
            }else if (redStones > 0) {
              redBoard[hole] +=1;totalSeeds[hole] +=1;redStones --;changeSeeds[hole]=1;
            }
        }else{
          if (redStones > 0) {
              redBoard[hole] +=1;totalSeeds[hole] +=1;redStones --;changeSeeds[hole]=1;
            }else if (blackStones > 0){
              blackBoard[hole] +=1;totalSeeds[hole] +=1;blackStones --;changeSeeds[hole]=2;
            }
          }
        hole ++;
        total --;
      }
      return hole;
    }

    //calculate the capture amount for each hole
    int captureValid(int hole,int &changeSeed){
      int res=0;//store the capture amount 
      //the capture rule related to the last-place seed
      switch(changeSeed)
      {
      case 1 : //red seed is the last
        if ((redBoard[hole]+specialBoard[hole])>1 && (redBoard[hole]+specialBoard[hole])<4){
          res=redBoard[hole]+specialBoard[hole];
          totalSeeds[hole] =blackBoard[hole];
          redBoard[hole]=0;
          specialBoard[hole]=0;
          changeSeed=1;}
        break;
      case 2 : //black seed is the last
        if ((blackBoard[hole]+specialBoard[hole])>1 && (blackBoard[hole]+specialBoard[hole])<4){
          res=blackBoard[hole]+specialBoard[hole];
          totalSeeds[hole] =redBoard[hole];
          blackBoard[hole]=0;
          specialBoard[hole]=0;
          changeSeed=2;}
        break;
      case 3 : //special seed is the last,one should regard as a red and a black.
        if ((redBoard[hole]+specialBoard[hole])>1&&(redBoard[hole]+specialBoard[hole])<4){
          if ((blackBoard[hole]+specialBoard[hole])>1&&(blackBoard[hole]+specialBoard[hole])<4){
            res=totalSeeds[hole];
            totalSeeds[hole] =0;
            blackBoard[hole]=0;
            specialBoard[hole]=0;
            redBoard[hole]=0;
            changeSeed=3;}
          else {          
            res=redBoard[hole]+specialBoard[hole];
            totalSeeds[hole] =blackBoard[hole];
            redBoard[hole]=0;
            specialBoard[hole]=0;
            changeSeed=1;}
          }
        else if ((blackBoard[hole]+specialBoard[hole])>1&&(blackBoard[hole]+specialBoard[hole])<4){
          res=blackBoard[hole]+specialBoard[hole];
          totalSeeds[hole] =redBoard[hole];
          blackBoard[hole]=0;
          specialBoard[hole]=0;
          changeSeed=2;
          }
      }
      return res;
    }

    // Check if the move is valid for the given player
    //  Output:
    //    bool: True if the move is valid
    bool checkValidMove1(int hole,int specialmove,bool playerIsFirst) {
      // Invalid if player picks hole on opposite side or from score hole
      int specialSeeds = specialBoard[hole-1];
      int total=totalSeeds[hole-1];
      int p=1;//to show if it has spacial seed;0-no;1-yes
      if (specialSeeds==0)  {p=0;}
      if (!playerIsFirst)  {hole= hole-6;}
      //if break one of these rules, a unvalid move!
      //the holes should be mine; the special move should not larger than the total; 
      if (hole <1 || hole > 6 ||(p*(specialmove+specialSeeds)) >total+1 ) { 
        return false;
      }
      return true;
    }

    // Check if the move is valid for the given player
    //  Output:
    //    bool: True if the move is valid
    bool checkValidMove2(int hole,bool playerIsFirst) {
      int total=totalSeeds[hole-1];
      int k=1;//to check if it all zeros for the opponent player;0-no,1-yes all zeros.
      int t=0;//the number to modify the checkpoint;if first player-0;if not-6
      if (!playerIsFirst)  {hole= hole-6;t=6;}
      //if all zeros for the opponent holes
      for (int i = 6; i < 12; i++) {
          if (totalSeeds[i-t] != 0) {
            k=0;break;
          }
      }
      //if break one of these rules, a unvalid move!
      //the holes should be mine; the special move should not larger than the total; if opponent starving, should move smth that make it not starving.
      if (k && (total+hole)<7) {
        return false;
      }
      return true;
    }

    // Print the current status of the board
    void printBoard() {
      cout << "\nCURRENT BOARD" << endl;
      cout << "Player 1's Side" << endl;
      cout << "           |  1   |  2   |  3   |   4  |   5  |   6  |" << endl;
	    cout << "_____________________________________________________________" << endl;
	    cout << "|" << "      | T | (" << std::setfill('0') << std::setw(2) << totalSeeds[0] << ") | " ;
	    cout << "(" << std::setfill('0') << std::setw(2)<< totalSeeds[1] ;
	    cout << ") | " << "(" << std::setfill('0') << std::setw(2)<<  totalSeeds[2] << ") | ";
	    cout << "(" << std::setfill('0') << std::setw(2)<<  totalSeeds[3] << ") | " ;
	    cout << "(" << std::setfill('0') << std::setw(2)<<  totalSeeds[4] << ") | " ;
	    cout <<"(" << std::setfill('0') << std::setw(2) << totalSeeds[5] << ") |      |" << endl;
      cout << "|" << "      | R | (" << std::setfill('0') << std::setw(2) << redBoard[0] << ") | " ;
	    cout << "(" << std::setfill('0') << std::setw(2)<< redBoard[1] ;
	    cout << ") | " << "(" << std::setfill('0') << std::setw(2)<<  redBoard[2] << ") | ";
	    cout << "(" << std::setfill('0') << std::setw(2)<<  redBoard[3] << ") | " ;
	    cout << "(" << std::setfill('0') << std::setw(2)<<  redBoard[4] << ") | " ;
	    cout <<"(" << std::setfill('0') << std::setw(2) << redBoard[5] << ") |      |" << endl;
      cout << "|" << "      | B | (" << std::setfill('0') << std::setw(2) << blackBoard[0] << ") | " ;
	    cout << "(" << std::setfill('0') << std::setw(2)<< blackBoard[1] ;
	    cout << ") | " << "(" << std::setfill('0') << std::setw(2)<<  blackBoard[2] << ") | ";
	    cout << "(" << std::setfill('0') << std::setw(2)<<  blackBoard[3] << ") | " ;
	    cout << "(" << std::setfill('0') << std::setw(2)<<  blackBoard[4] << ") | " ;
	    cout <<"(" << std::setfill('0') << std::setw(2) << blackBoard[5] << ") |      |" << endl;
      cout << "|" << "      | S | (" << std::setfill('0') << std::setw(2) << specialBoard[0] << ") | " ;
	    cout << "(" << std::setfill('0') << std::setw(2)<< specialBoard[1] ;
	    cout << ") | " << "(" << std::setfill('0') << std::setw(2)<<  specialBoard[2] << ") | ";
	    cout << "(" << std::setfill('0') << std::setw(2)<<  specialBoard[3] << ") | " ;
	    cout << "(" << std::setfill('0') << std::setw(2)<<  specialBoard[4] << ") | " ;
	    cout <<"(" << std::setfill('0') << std::setw(2) << specialBoard[5] << ") |      |" << endl;   

	    cout << "|      |‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|      |" << endl;

	    cout << "|  " << std::setfill('0') << std::setw(2) << scoreBoard[0] << "  |";
	    cout << "                                             ";
	    cout << "|  " << std::setfill('0') << std::setw(2) << scoreBoard[1] << "  |" << endl;
	    cout << "|      |_____________________________________________|      |" << endl;

	    cout << "|" << "      | T | (" << std::setfill('0') << std::setw(2) << totalSeeds[11] << ") | " ;
	    cout << "(" << std::setfill('0') << std::setw(2)<< totalSeeds[10] ;
	    cout << ") | " << "(" << std::setfill('0') << std::setw(2)<<  totalSeeds[9] << ") | ";
	    cout << "(" << std::setfill('0') << std::setw(2)<<  totalSeeds[8] << ") | " ;
    	cout << "(" << std::setfill('0') << std::setw(2)<<  totalSeeds[7] << ") | " ;
	    cout <<"(" << std::setfill('0') << std::setw(2) << totalSeeds[6] << ") |      |" << endl;
	    cout << "|" << "      | R | (" << std::setfill('0') << std::setw(2) << redBoard[11] << ") | " ;
	    cout << "(" << std::setfill('0') << std::setw(2)<< redBoard[10] ;
	    cout << ") | " << "(" << std::setfill('0') << std::setw(2)<<  redBoard[9] << ") | ";
	    cout << "(" << std::setfill('0') << std::setw(2)<<  redBoard[8] << ") | " ;
    	cout << "(" << std::setfill('0') << std::setw(2)<<  redBoard[7] << ") | " ;
	    cout <<"(" << std::setfill('0') << std::setw(2) << redBoard[6] << ") |      |" << endl;
      cout << "|" << "      | B | (" << std::setfill('0') << std::setw(2) << blackBoard[11] << ") | " ;
	    cout << "(" << std::setfill('0') << std::setw(2)<< blackBoard[10] ;
	    cout << ") | " << "(" << std::setfill('0') << std::setw(2)<<  blackBoard[9] << ") | ";
	    cout << "(" << std::setfill('0') << std::setw(2)<<  blackBoard[8] << ") | " ;
    	cout << "(" << std::setfill('0') << std::setw(2)<<  blackBoard[7] << ") | " ;
	    cout <<"(" << std::setfill('0') << std::setw(2) << blackBoard[6] << ") |      |" << endl;
      cout << "|" << "      | S | (" << std::setfill('0') << std::setw(2) << specialBoard[11] << ") | " ;
	    cout << "(" << std::setfill('0') << std::setw(2)<< specialBoard[10] ;
	    cout << ") | " << "(" << std::setfill('0') << std::setw(2)<<  specialBoard[9] << ") | ";
	    cout << "(" << std::setfill('0') << std::setw(2)<<  specialBoard[8] << ") | " ;
    	cout << "(" << std::setfill('0') << std::setw(2)<<  specialBoard[7] << ") | " ;
	    cout <<"(" << std::setfill('0') << std::setw(2) << specialBoard[6] << ") |      |" << endl;
	    cout << "‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾" <<endl;
	    cout << "           |  12  |  11  |  10  |  9   |   8  |   7  |" << endl;
      cout << "                                              Player 2's Side\n\n";
    }


    // Check if the game has ended (if one player's side has been depleted)
    //The game is over when one player has captured 38 or more seeds, or each player has taken 37 seeds (draw). 
    bool isGameOver() {
      if (scoreBoard[0]>37||scoreBoard[1]>37){
        printBoard();
        cout <<"Game end!"<<endl;
        return true;}
      if (scoreBoard[0]==37&&scoreBoard[1]==37){
        printBoard();
        cout <<"Game end without winner!"<<endl;
        return true;}
      return false;
    }

    //Deep copy
    OwareBoard * copy(){
      OwareBoard* t = new OwareBoard();
      for (int i = 0; i < NUM_HOLES; i++) {
        t->redBoard[i] = this->redBoard[i];
        t->blackBoard[i] =this->blackBoard[i];
        t->specialBoard[i] = this->specialBoard[i];
        t->changeSeeds[i] = this->changeSeeds[i];
        t->totalSeeds[i] = this->totalSeeds[i];
      }
      t->scoreBoard[0] = this->scoreBoard[0];
      t->scoreBoard[1] = this->scoreBoard[1];
      t->playerFirst=this->playerFirst;
      return t;
    }
    
    // search and show the recommend movement rule
    int searching(){
		    Node node;
		    int MaxD =2;
	  	  int alpha = -999;
		    int beta = 999;
		    int minOrMax = 1;
        int bestHole,tbestHole,bestScore,tbestScore,tscore;
        bool bestColor,tbestColor;
        int bestSpecialmove,score,tbestSpecialmove;
        int change=6;
        if (playerFirst) change=0; 
        signal1=0;
        double tim=0;
        struct  timeval start;
        struct  timeval end;
        gettimeofday(&start,NULL);
        std::thread t(setTime);
        t.detach();
        gettimeofday(&end,NULL);
        while(1){
            std::tie(score,bestScore)=minmax(node,MaxD,MaxD,alpha,beta,minOrMax, bestHole, bestColor, bestSpecialmove,bestScore);
            gettimeofday(&end,NULL);
            if (score==-99) break;
            MaxD++;
            tscore=-score;
            tbestScore=bestScore;
            tbestColor=bestColor;
            tbestHole=bestHole;
            tbestSpecialmove=bestSpecialmove;
            if (tbestScore>37) break;
            if (MaxD<6) MaxD++;
        } 
        cout<<"Score:"<<tbestScore<<" Evaluated Score:"<<tscore<<"  running time :"<<(1000000*(end.tv_sec-start.tv_sec)+ end.tv_usec-start.tv_usec)<<" ms"<<endl;
        char col;
        if (tbestColor) col='B';
        else col='R';
        cout << "depth is "<<MaxD-1<<" and Optimal move at "<< (tbestHole+1) <<col<<tbestSpecialmove << endl;
        
		return tscore;
      
    }


    int evalFunction(int minOrMax){
        int score = scoreBoard[0]-scoreBoard[1];
        score = score*minOrMax;
    return score;
    }

    std::tuple<int,int> minmax(Node node, int depth,int MaxD, int alpha, int beta,int minOrMax, int&bestHole, bool&bestColor, int&bestSpecialmove, int&bestScore){
      if (signal1) {
          return std::make_tuple(-99,0);
      }
        int alphaOrig=alpha;
        unsigned long long int ttt=geneKey(this);
        hasht ifhit = FindHashTable(ttt);

if (ifhit.key !=NULLKEY && (ifhit.depth >= depth)) {
    switch (ifhit.bound){
        case Exact:
            node.score=ifhit.bestmove.score;
            node.bestScore=ifhit.bestmove.bestScore;
            node.hole=ifhit.bestmove.hole;
            node.color=ifhit.bestmove.color;
            return std::make_tuple(-node.score,node.bestScore);
        case Alpha:
            if(ifhit.bestmove.score > alpha)
                alpha=ifhit.bestmove.score;
            break;
        case Beta:
            if(ifhit.bestmove.score < beta)
                beta=ifhit.bestmove.score;
            break;
    }
    if (alpha>=beta){
            node.score=ifhit.bestmove.score;
            node.bestScore=ifhit.bestmove.bestScore;
            node.hole=ifhit.bestmove.hole;
            node.color=ifhit.bestmove.color;
        return std::make_tuple(-node.score,node.bestScore);
    }
}
	    int goAgain,k,i,change, bestHole2,bestSpecialmove2,bestScore2;
      bool playerIsFirst,bestColor2;
	    Node child;
	    if(depth == 0){
		      node.score = playerFirst * evalFunction(minOrMax);
              if (playerFirst==1)
                node.bestScore=scoreBoard[0];
              else node.bestScore=scoreBoard[1];
		      return  std::make_tuple(-node.score,node.bestScore);
	    }else{  
              OwareBoard* tp = this->copy();
			        if(tp->playerFirst*minOrMax>0)
				      {change=0; playerIsFirst=true;}
			        else
				      {change=6;playerIsFirst=false;}	
			        for(i=change;i<change+6;i++){
				        if(tp->totalSeeds[i]!=0){
					        child.hole = i;
					      if(tp->redBoard[i]!=0){
						      child.color = false;
						      if(tp->specialBoard[i]!=0){
							      for(k=1; (k+tp->specialBoard[i]-1)<(tp->totalSeeds[i]+1); k++){
								      child.specialmove = k;
                      OwareBoard* tpchild = tp->copy();
								      goAgain = tpchild->updateBoard(child.hole+1, child.color, child.specialmove,playerIsFirst);
								      if(!goAgain){
									      std::tie(child.score,child.bestScore) = tpchild->minmax(child, depth-1,MaxD,-beta,-alpha, -minOrMax, bestHole, bestColor, bestSpecialmove,bestScore);
                        if (signal1) {
                          delete tpchild;
                          delete tp;
                            return std::make_tuple(-99,0);
                        }
									      if(child.score>alpha){ 
										      alpha = child.score;
                          bestHole2 = child.hole;
                          bestColor2 = child.color;
                          bestSpecialmove2 = child.specialmove;
                          bestScore2= child.bestScore;
                        }
									      if (beta<=alpha)
										      break;
								      }	
                      delete tpchild;
							      }
						      }
						      else{
                      child.specialmove = 0;
                      OwareBoard* tpchild = tp->copy();
								      goAgain = tpchild->updateBoard(child.hole+1, child.color, child.specialmove,playerIsFirst);
								      if(!goAgain){
									        std::tie(child.score,child.bestScore)  = tpchild->minmax(child, depth-1,MaxD,  -beta,-alpha, -minOrMax, bestHole, bestColor, bestSpecialmove,bestScore);
                          if (signal1) {
                            delete tpchild;
                            delete tp;
                            return std::make_tuple(-99,0);
                        }X
									        if(child.score>alpha){ 
										      alpha = child.score;
                          bestHole2 = child.hole;
                          bestColor2 = child.color;
                          bestSpecialmove2 = child.specialmove;
                          bestScore2= child.bestScore;
                        }
									      if(beta<alpha)
										      break;
						          }
                      delete tpchild;
                    }
                }
						    if(tp->blackBoard[i]!=0){
							    child.color = true;
							    if(tp->specialBoard[i]!=0){
								      for( k=1; (k+tp->specialBoard[i]-1)<(tp->totalSeeds[i]+1); k++){
									        child.specialmove = k;
                          OwareBoard* tpchild = tp->copy();
								          goAgain = tpchild->updateBoard(child.hole+1, child.color, child.specialmove,playerIsFirst);
								          if(!goAgain){
										        std::tie(child.score,child.bestScore)  = tpchild->minmax(child,  depth-1,MaxD, -beta,-alpha, -minOrMax, bestHole, bestColor, bestSpecialmove,bestScore);
                            if (signal1) {
                            delete tpchild;
                            delete tp;
                            return std::make_tuple(-99,0);
                        }
										        if(child.score>alpha){ 
											        alpha = child.score;
                              bestHole2 = child.hole;
                              bestColor2 = child.color;
                              bestSpecialmove2 = child.specialmove;
                              bestScore2= child.bestScore;
                            }
										        if(beta<=alpha)
											        break;
									        }
                          delete tpchild;
								      }
							    }else{
                      child.specialmove = 0;
                      OwareBoard* tpchild = tp->copy();
								      goAgain = tpchild->updateBoard(child.hole+1, child.color, child.specialmove,playerIsFirst);
								      if(!goAgain){
									        std::tie(child.score,child.bestScore) = tpchild->minmax(child,  depth-1,MaxD,-beta,-alpha, -minOrMax, bestHole, bestColor, bestSpecialmove,bestScore);
                          if (signal1) {
                            delete tpchild;
                            delete tp;
                            return std::make_tuple(-99,0);
                        }
									        if(child.score>alpha){
										    alpha = child.score;
                              bestHole2 = child.hole;
                              bestColor2 = child.color;
                              bestSpecialmove2 = child.specialmove;
                              bestScore2= child.bestScore;
                          }
									        if(beta<=alpha)
										        break;
								      }
                      delete tpchild;
							    }  
						    }		
				      }
			      }
            delete tp;
            bestHole = bestHole2;
            bestColor = bestColor2;
            bestSpecialmove = bestSpecialmove2;
            bestScore= bestScore2;
		    }
        
    ScoreType bound;
    if (alpha <= alphaOrig)
        bound= Beta;
    else if (alpha >= beta) 
        bound= Alpha;
    else
        bound= Exact;
    insertHashTable(ttt,depth, node, bound);
        return std::make_tuple(-alpha,bestScore);
	    }


    void minmax_special(){
      	int alpha=-99;
		int goAgain,k,i,change;
      k=-99;goAgain=1;
      OwareBoard* tp = this->copy();
		  if(tp->playerFirst>0)
			  {
#pragma omp parallel for num_threads(6)
				for(i=0;i<6;i++){
        			tp->specialBoard[i]=1;
					for (int j=6;j<12;j++){
						tp->specialBoard[j]=1;
						alpha=tp->searching();
        				if (alpha>k){
            				goAgain=i+1;
            				k=alpha;
       		 			}
			  		}
				}
				cout<<"special place:"<<goAgain<<endl;
				}
		  else{ 
        goAgain=7;
#pragma omp parallel for num_threads(6)	ordered		  
					for (int j=6;j<12;j++){
						tp->specialBoard[j]=1;
            tp->updateBoard(1,false,1,true);
						alpha=tp->searching();
            cout<<alpha<<endl;
#pragma omp ordered
        				if (alpha>k){
            				goAgain=j+1;
            				k=alpha;
       		 			}
			  		}
					cout<<"special place:"<<goAgain<<endl;
			}
			delete tp;
	}

    };



void HashEntry(hasht ht,unsigned long long int hash, int depth, int stale, Node bestMove, ScoreType bound)
    {
        ht.key = hash;
        ht.depth = depth;
        ht.bestmove = bestMove;
        ht.stale=stale;
        ht.bound = bound;
    }

unsigned long long int hstable1[12][37][37][3];
unsigned long long int hstable2[2][38];

void TranspositionTable(unsigned long long int size) {
	thash.arr = (hasht*)malloc(sizeof(hasht)*size);
    thash.size = size;
}

void initTable(){
    srand((unsigned)time(NULL));
    for (int i=0;i<12;i++){
        for (int j=0;j<37;j++){
            for (int l=0;l<37;l++){
                for (int d=0;d<3;d++){
                    hstable1[i][j][l][d]=rand();
                }
            }
        }
    }
    srand((unsigned)time(NULL));
    for (int i=0;i<2;i++){
        for (int j=0;j<38;j++){
            hstable2[i][j]=rand();
        }
    }
}

unsigned long long int geneKey(OwareBoard *t){
    unsigned long long int h = 0; 
    for (int i=0;i<12;i++){
       int r=t->redBoard[i];
       int b=t->blackBoard[i];
       int s=t->specialBoard[i];
       h ^= hstable1[i][r][b][s];
    }
    for (int i=0;i<2;i++){
       int r=t->scoreBoard[i];
       h ^= hstable2[i][r];
    }
    return h;
} 

hasht FindHashTable(unsigned long long int t){
    int i = t%thash.size;
    int j = i;
    if (thash.arr[j].key==NULLKEY)  {
        thash.arr[j].stale++;
   } else thash.arr[j].stale=0;
   return thash.arr[j];
}

void insertHashTable(unsigned long long int t,int depth, Node bestMove, ScoreType bound){
    int j=t%thash.size;
    if(thash.arr[j].key==NULLKEY||thash.arr[j].stale>2||thash.arr[j].depth<depth||bound==Exact)
        HashEntry(thash.arr[j],t, depth, 0, bestMove, bound);
}


int main() {
  OwareBoard * b = new OwareBoard();
  TranspositionTable(1024*1024);
  initTable();
  int tmp;// store which hole should place a special seed
  cout << "Who play first? 1:Mine; -1:Competitor" << endl;
  cin >> b->playerFirst;
  if (b->playerFirst==1) 
              cout<<"Place at 1\n";
  //          b->minmax_special();//use the minmax function to find the best move
  cout << "Player 1: Select the hole to place the special seed (hole from 1 to 6)." << endl;
  cin >> tmp;
  b->specialBoard[tmp-1]=1;
  b->totalSeeds[tmp-1] ++;
  if (b->playerFirst==-1) 
    cout<<"Place at 7\n";
         //b->minmax_special();//use the minmax function to find the best move
  cout << "Player 2: Select the hole to place the special seed (hole from 7 to 12)." << endl;
  cin >> tmp;
  b->specialBoard[tmp-1]=1;   
  b->totalSeeds[tmp-1] ++;  
  b->playGame();
  return 0;
}
