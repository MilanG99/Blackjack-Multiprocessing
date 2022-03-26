/***************************************************************************
* File: blackjack_pipes.cpp
* Author: Milan Gulati
* Procedures:
* main          - creates pipes and forks dealer and player processes, manages the processes
* playerOne     - player one hit/stand strategy (hit when < 15)
* playerTwo     - player two hit/stand strategy (hit when < 18)
* dealer        - dealer hit/stand rules (hit when < 17)
* handValue     - computes integer value of hand vector
***************************************************************************/

/* Import Libraries */
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <bits/stdc++.h>
#include <unistd.h>

using namespace std;

/* Function Prototypes */
bool playerOne(int val);        // player one strategy function
bool playerTwo(int val);        // player two strategy function
bool dealer(int val);           // dealer rules function
int handValue(vector<char> hand);   // compute hand value function

/***************************************************************************
* int main()
* Author: Milan Gulati
* Description: Creates pipes, dealer and player processes. The dealer (manager) process
*              and two player (worker) processes are managed in the main method.
*              The dealer process manages the players by sending them cards via pipes 
*              based on their response to their hand. Tracks the wins of the dealer and 
*              players.
*
* Parameters:
*   argc    I/P     int         Number of arguments on command line
*   argv    I/P     char *[]    Arguments listed on command line
*   main    O/P     int         Status code returns 1 on failure of fork() or pipe() system calls
***************************************************************************/
int main(int argc, char *argv[])
{
    /*
    * cards[] is a character array of each possible card drawn
    * 'A' represents Ace for a value of either 1 or 11
    * 'T' represents 10, Jack, Queen, King for a value of 10
    *  All other cards are at face value
    */
    char cards[] = {'2', '2', '2', '2', '3', '3', '3', '3', '4', '4', '4', '4', '5', '5', '5', '5',
                    '6', '6', '6', '6', '7', '7', '7', '7', '8', '8', '8', '8', '9', '9', '9', '9',
                    'T', 'T', 'T', 'T', 'T', 'T', 'T', 'T', 'T', 'T', 'T', 'T', 'T', 'T', 'T', 'T',
                    'A', 'A', 'A', 'A'};

    int dealerWins = 0; // track dealer wins
    int p1Wins = 0;     // track player 1 wins
    int p2Wins = 0;     // track player 2 wins
    int spot = 0;       // track "spot" in deck after sending/drawing a card

    /* Declare File Descriptors */
    int fd_cards_p1[2]; // pipe to send cards to player 1
    int fd_cards_p2[2]; // pipe to send cards to player 2
    int fd_hs_p1[2];    // pipe to receive hit/stand from player 1
    int fd_hs_p2[2];    // pipe to receive hit/stand from player 2

    /* Open Pipes */
    // pipe function returns -1 on failure
    if(pipe(fd_cards_p1) == -1){
        return 1;
    }
    if(pipe(fd_cards_p2) == -1){
        return 1;
    }
    if(pipe(fd_hs_p1) == -1){
        return 1;
    }
    if(pipe(fd_hs_p2) == -1){
        return 1;
    }       
    
    /* First fork() */
    pid_t pid = fork();

    if(pid < 0) // fork function returns negative on failure
    {
        return 1;
    }

    /* Parent Dealer Process */
    else if(pid > 0)
    {
        close(fd_cards_p1[0]);                              // close reading end of card pipe for p1
        close(fd_cards_p2[0]);                              // close reading end of card pipe for p2
        close(fd_hs_p1[1]);                                 // close writing end of h/s pipe for p1
        close(fd_hs_p2[1]);                                 // close writing end of h/s pipe for p2

        vector<char> handDealer;                            // dealer's hand vector
        int valDealer = 0;                                  // value of dealer's hand
        bool statusDealer;                                  // hit/stand for dealer

        for(int i = 0; i < 1000; i++)
        {
            srand(time(0));                                 // set random seed
            random_shuffle(cards, cards + 52);              // shuffle for new iteration
            spot = 0;                                       // top of deck

            handDealer.clear();                             // clear dealer's hand

            /* Deal Initial Hand */
            // add two cards to dealer's hand
            handDealer.push_back(cards[spot]);              // add card to dealer hand
            spot++;                                         // next card
            handDealer.push_back(cards[spot]);              // add card to dealer hand
            spot++;                                         // next card
            // send two cards to p1
            write(fd_cards_p1[1], &cards[spot], 1);         // send card to p1
            spot++;                                         // next card
            write(fd_cards_p1[1], &cards[spot], 1);         // send card to p1
            spot++;                                         // next card
            // send two cards to p2
            write(fd_cards_p2[1], &cards[spot], 1);         // send card to p2
            spot++;                                         // next card
            write(fd_cards_p2[1], &cards[spot], 1);         // send card to p2
            spot++;                                         // next card

            /* Hit/Stand Response */
            // p1 sends hit or stand signal --> send card back until stand
            // p2 sends hit or stand signal --> send card back until stand
            bool statusP1, statusP2;                        // stores hit/stand signal from players
            read(fd_hs_p1[0], &statusP1, 1);                // hit/stand signal from p1
            read(fd_hs_p2[0], &statusP2, 1);                // hit/stand signal from p2

            // send cards to p1 until stand
            while(statusP1 == true)
            {
                write(fd_cards_p1[1], &cards[spot], 1);     //send card
                spot++;
                read(fd_hs_p1[0], &statusP1, 1);            // recieve updated status
            }
            // send cards to p2 until stand
            while(statusP2 == true)
            {
                write(fd_cards_p2[1], &cards[spot], 1);     //send card
                spot++;
                read(fd_hs_p2[0], &statusP2, 1);            // recieve updated status
            }

            /* Dealer Draws Cards */
            valDealer = handValue(handDealer);              // compute dealer's hand value
            statusDealer = dealer(valDealer);               // determine dealer's status
            while(statusDealer == true)                     // hit while status is true
            {
                handDealer.push_back(cards[spot]);          // add card to hand
                spot++;
                valDealer = handValue(handDealer);          // recompute hand value
                statusDealer = dealer(valDealer);           // recompute status of dealer
            }

            int valP1, valP2;                               // store final value of player hands
            read(fd_hs_p1[0], &valP1, 4);                   // recieve final hand value of P1
            read(fd_hs_p2[0], &valP2, 4);                   // recieve final hand value of P2

            /* Determine Wins */
            if(valDealer > 21)                              // dealer busts
            {
                if(valP1 <= 21)                             // dealer busts, player1 <= 21, player1 wins
                    p1Wins++;
                if(valP2 <= 21)                             // dealer busts, player2 <= 21, player2 wins
                    p2Wins++;
                if(valP1 > 21 || valP2 > 21)                // either player busts, dealer wins
                    dealerWins++;
            }
            else                                            // dealer <= 21
            {
                if(valP1 <= 21 && valP1 > valDealer)        // player1 <= 21 and player > dealer, player1 wins
                    p1Wins++;
                if(valP2 <= 21 && valP2 > valDealer)        // player2 <= 21 and player > dealer, player2 wins
                    p2Wins++;
                if(valP1 < valDealer || valP2 < valDealer)  // dealer <= 21 and player < dealer, dealer wins
                    dealerWins++;
            }
        }

        close(fd_cards_p1[1]);                              // close writing side card pipe p1
        close(fd_cards_p2[1]);                              // close writing side card pipe p2
        close(fd_hs_p1[0]);                                 // close reading side hit/stand pipe p1
        close(fd_hs_p2[0]);                                 // close reading side hit/stand pipe p2        

        // 1000 games have finished 
        // display win stats for players and dealer
        cout << "\nPIPE IMPLEMENTATION" << endl;
        cout << "Games:             1000" << endl;
        cout << "----------------------------------------------" << endl;
        cout << "Player One Wins:   " << p1Wins << " | Win Precentage: " << setprecision(4) << p1Wins/10.0 << "%" << endl;
        cout << "Player Two Wins:   " << p2Wins << " | Win Precentage: " << setprecision(4) << p2Wins/10.0 << "%" << endl;
        cout << "Dealer Wins:       " << dealerWins << " | Win Precentage: " << setprecision(4) <<dealerWins/10.0 << "%" << endl;

        return 0;
    }

    /* Worker Player Processes */
    else
    {
        /* Second fork() */
        pid_t pid2 = fork();

        if(pid2 < 0)                                    // fork failure
        {
            return 1;
        }
        
        /* Player 1 Process */
        else if(pid2 > 0)
        {
            vector<char> handP1;                        // p1 hand vector
            char c1, c2;                                // first two cards from dealer
            int valP1 = 0;                              // value of hand
            bool hitStand = false;                      // hit or stand determination

            close(fd_cards_p1[1]);                      // close writing end of card pipe for p1
            close(fd_hs_p1[0]);                         // close reading end of hs pipe for p1

            // iterations must be the same amount as parent for loop (1000)
            for(int p1 = 0; p1 < 1000; p1++)
            {
                handP1.clear();                         // clear hand

                read(fd_cards_p1[0], &c1, 1);           // read first card
                read(fd_cards_p1[0], &c2, 1);           // read second card
                handP1.push_back(c1);                   // add first card to vector
                handP1.push_back(c2);                   // add second card to vector

                valP1 = handValue(handP1);              // calculate current total

                hitStand = playerOne(valP1);
                write(fd_hs_p1[1], &hitStand, 1);       // send initial hit/stand signal
                while(hitStand == true)                 // while hit is true
                {
                    char temp;
                    read(fd_cards_p1[0], &temp, 1);     // recieve one more card
                    handP1.push_back(temp);             // add card to hand
                    valP1 = handValue(handP1);          // recompute hand value
                    hitStand = playerOne(valP1);        // redetermine status
                    write(fd_hs_p1[1], &hitStand, 1);   // send hit signal to dealer via fd_hs_p1
                }

                write(fd_hs_p1[1], &valP1, 4);          // send final hand value to dealer
            }

            close(fd_cards_p1[0]);                      // close reading side card pipe p1
            close(fd_hs_p1[1]);                         // close writing side hit/stand pipe p1

            exit(0);                                    // exit completed process
        }

        /* Player 2 Process */
        else
        {
            vector<char> handP2;                        // p2 hand vector
            char c1, c2;                                // first two cards from dealer
            int valP2 = 0;                              // value of hand
            bool hitStand = false;                      // hit or stand determination

            close(fd_cards_p2[1]);                      // close writing end of card pipe for p2
            close(fd_hs_p2[0]);                         // close reading end of hs pipe for p2

            // iterations must be the same as parent for loop (1000)
            for(int p2 = 0; p2 < 1000; p2++)
            {
                handP2.clear();                         // clear hand

                read(fd_cards_p2[0], &c1, 1);           // read first card
                read(fd_cards_p2[0], &c2, 1);           // read second card
                handP2.push_back(c1);                   // add first card to vector
                handP2.push_back(c2);                   // add second card to vector

                valP2 = handValue(handP2);              // calculate current total

                hitStand = playerTwo(valP2);
                write(fd_hs_p2[1], &hitStand, 1);       // send initial hit/stand signal
                while(hitStand == true)                 // while hit is true
                {
                    char temp;
                    read(fd_cards_p2[0], &temp, 1);     // recieve one more card
                    handP2.push_back(temp);             // add card to hand
                    valP2 = handValue(handP2);          // recompute hand value
                    hitStand = playerTwo(valP2);        // redetermine status
                    write(fd_hs_p2[1], &hitStand, 1);   // send hit signal to dealer via fd_hs_p2
                }

                write(fd_hs_p2[1], &valP2, 4);          // send final hand value to dealer
            }

            close(fd_cards_p2[0]);                      // close reading side card pipe p2
            close(fd_hs_p2[1]);                         // close writing side hit/stand pipe p2

            exit(0);                                    // exit completed process
        }
    }
}

/***************************************************************************
* int playerOne(int val)
* Author: Milan Gulati
* Description: Player one's hit/stand strategy. Hits when less than 15.
*              Stands when greater than equal to 15.
*              Hit returns true. Stand returns false.
*
* Parameters:
*   val         I/P     int     Integer value of current hand  
*   playerOne   O/P     bool    Hit or stand signal for player one process
***************************************************************************/
bool playerOne(int val)
{
    if(val < 15)    // hit if hand < 15
        return true;
    return false;   // stand otherwise
}

/***************************************************************************
* int playerTwo(int val)
* Author: Milan Gulati
* Description: Player two's hit/stand strategy. Hits when less than 18.
*              Stands when greater than equal to 18.
*              Hit returns true. Stand returns false.
*
* Parameters:
*   val         I/P     int     Integer value of current hand  
*   playerTwo   O/P     bool    Hit or stand signal for player two process
***************************************************************************/
bool playerTwo(int val)
{
    if(val < 18)    // hit if hand < 18
        return true;
    return false;   // stand otherwise
}

/***************************************************************************
* int dealer(int val)
* Author: Milan Gulati
* Description: Dealer's hit/stand rule. Hits when less than 17.
*              Stands when greater than equal to 17.
*              Hit returns true. Stand returns false.
*
* Parameters:
*   val         I/P     int     Integer value of current hand  
*   dealer      O/P     bool    Hit or stand signal for dealer process
***************************************************************************/
bool dealer(int val)
{
    if(val < 17)    // hit if hand < 17
        return true;
    return false;   // stand otherwise
}

/***************************************************************************
* int handValue(vector<char> hand)
* Author: Milan Gulati
* Description: Computes the integer value of the hand vector argument. Used
*              Aces are treated specially and are dependent on the value of
*              the other cards in the hand.
*
* Parameters:
*   hand        I/P     vector<char>    Vector of chars containing cards in current hand 
*   handValue   O/P     int             Integer value of hand computed from vector hand
***************************************************************************/
int handValue(vector<char> hand)
{
    int sum = 0;                                        // running sum of hand
    int aces = count(hand.begin(), hand.end(), 'A');    // occurences of 'A' in vector

    // if aces exist in hand
    if(aces > 0)
    {
        // iterate through hand and sum non-ace values
        for(auto c: hand)
        {
            if(c != 'A')                                // card must not be an ace
            {
                if(c == 'T')                            // 'T' equivalent to 10,J,Q,K
                    sum += 10;                          // add to sum
                else                                    // treat all other cards as face value
                {
                    int val = c - '0';                  // convert char to int
                    sum += val;                         // add to sum
                }
            }
        }

        // remaining cards are aces
        // depending on how many aces are present
        // the values will be treated as 1 for each ace accordingly
        switch (aces)
        {
        case 1: /* One Ace: 1 or 11 */
            if(sum <= 10)                               // if existing sum is able to fit 11 (blackjack if sum = 10!)
                sum += 11;
            else                                        // else treat aces as 1
                sum += 1;
            break;
        case 2: /* Two Ace: 2 or 12 */
            if(sum <= 9)                                // existing sum able to fit 12 (11, 1)
                sum += 12;
            else                                        // else treat aces as 1 (1, 1)
                sum += 2;
            break;
        case 3: /* Three Ace: 3 or 13 */
            if(sum <= 8)                                // existing sum able to fit 13 (11, 1, 1)
                sum += 13;
            else                                        // else treat aces as 1 (1, 1, 1)
                sum += 3;
            break;
        case 4: /* Four Ace: 4 or 14 */
            if(sum <= 7)                                // existing sum able to fit 14 (11, 1, 1, 1)
                sum += 14;
            else                                        // else treat aces as 1 (1, 1, 1, 1)
                sum += 4;
            break;
        default:                                        // default case will not occur
            break;
        }
    }

    // else no aces exist in hand
    else 
    {
        // iterate though hand and sum up every value
        // no need to account for aces in this case
        for(auto c: hand)
        {
            if(c == 'T')                                // 'T' equivalent to 10,J,Q,K
                sum += 10;                              // add to sum
            else                                        // treat all other cards as face value
            {
                int val = c - '0';                      // convert char to int
                sum += val;                             // add to sum
            }
        }
    }
    return sum;                                         // return sum of hand
}
