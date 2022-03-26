/***************************************************************************
* File: blackjack_mq.cpp
* Author: Milan Gulati
* Procedures:
* main          - creates message queues and forks dealer and player processes, manages the processes
* playerOne     - player one hit/stand strategy (hit when < 15)
* playerTwo     - player two hit/stand strategy (hit when < 18)
* dealer        - dealer hit/stand rules (hit when < 17)
* handValue     - computes integer value of hand vector
***************************************************************************/

/* Import Libraries */
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <bits/stdc++.h>
#include <algorithm>
#include <iomanip>
#include <iostream>

using namespace std;

/* Function Prototypes */
bool playerOne(int val);                    // player one strategy function
bool playerTwo(int val);                    // player two strategy function
bool dealer(int val);                       // dealer rules function
int handValue(vector<char> hand);           // compute hand value function

// message buffer for card chars
struct cardbuff
{
    long msg_type;                          // message type (1 for char)
    char card;                              // card char representation
};

// message buffer for hit/stand bool
struct hsbuff
{
    long msg_type;                          // message type (2 for bool)
    bool hs;                                // hit or stand bool
};

// message buffer for hand value
struct handbuff
{
    long msg_type;                          // message type (3 for int)
    int hand;                               // hand integer value
};

/***************************************************************************
* int main()
* Author: Milan Gulati
* Description: Creates message queues, dealer and player processes. The dealer
*              manages the two worker player processes. The dealer process manages
*              players by sending cards via message queues for the initial hand,
*              as well as in response to the player's decision to hit/stand, 
*              which is also sent via messsage queue. The dealer recieves the value
*              for the player's final hand through another message queue. Tracks the 
*              wins of dealer and players.
*
* Parameters:
*   argc    I/P     int         Number of arguments on command line
*   argv    I/P     char *[]    Arguments listed on command line
*   main    O/P     int         Status code returns nothing
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
                    
    int dealerWins = 0;                                 // track dealer wins
    int p1Wins = 0;                                     // track player 1 wins
    int p2Wins = 0;                                     // track player 2 wins
    int spot = 0;                                       // track "spot" in deck after sending/drawing a card

    /*
    * Declare Keys
    * ftok creates a unique key_t to be used for IPC
    * different processes can access the same resources (message queues) given the same key_t
    * the key is used by msgget() to obtain an integer id to refer to a specific message queue
    * this id is used when sending and receiving messages from a queue
    * key_t ftok(const char *pathname, int proj_id)
    * pathname is the location of the source file for each key
    * proj_id must be unique for each ftok to generate a unique key
    */
    key_t card1 = ftok("./programtwo_mq.cpp", 66);      // identifier for cards sent to p1
    key_t card2 = ftok("./programtwo_mq.cpp", 69);      // identifier for cards sent to p2
    key_t hs1 = ftok("./programtwo_mq.cpp", 72);        // identifier for hit/stand sent from p1 to dealer
    key_t hs2 = ftok("./programtwo_mq.cpp", 75);        // identifier for hit/stand sent from p2 to dealer
    key_t hand1 = ftok("./programtwo_mq.cpp", 78);      // identifier for hand value sent from p1 to dealer
    key_t hand2 = ftok("./programtwo_mq.cpp", 81);      // identifier for hand value sent from p2 to dealer

    /*
    * Set msgget()
    * msgget will provide a unique integer for each message queue
    * int msgget(key_t, int msgflg)
    * key_t used is the respecitve key above
    * msgflg is set to 0666 | IPC_CREAT to set permissions and create new queue
    */
    int id_card1 = msgget(card1, 0666 | IPC_CREAT);     // create specific message queue for card1
    int id_card2 = msgget(card2, 0666 | IPC_CREAT);     // create specific message queue for card2
    int id_hs1 = msgget(hs1, 0666 | IPC_CREAT);         // create specific message queue for hs1
    int id_hs2 = msgget(hs2, 0666 | IPC_CREAT);         // create specific message queue for hs2
    int id_hand1 = msgget(hand1, 0666 | IPC_CREAT);     // create specific message queue for hand1
    int id_hand2 = msgget(hand2, 0666 | IPC_CREAT);     // create specific message queue for hand2

    /* First fork() */
    pid_t pid = fork();

    if(pid < 0)                                         // fork function returns negative on failure
    {
        return 1;
    }

    /* Parent Dealer Process */
    else if(pid > 0)
    {
                                                        // declare structs for sending cards, recieving hit/stand, recieving hand values
        cardbuff card_p1, card_p2;                      // cards to p1, p2
        hsbuff hs_p1, hs_p2;                            // h/s from p1, p2
        handbuff hand_p1, hand_p2;                      // hand from p1, p2

        vector<char> handDealer;                        // dealer's hand vector
        int valDealer = 0;                              // value of dealer's hand
        bool statusDealer;                              // hit/stand for dealer

        for(int i = 0; i < 1000; i++)
        {
            srand(time(0));                             // set random seed
            random_shuffle(cards, cards + 52);          // shuffle for new iteration
            spot = 0;                                   // top of deck

            handDealer.clear();                         // clear dealer's hand

            /* Deal Initial Hand */
                                                        // add two cards to dealer's hand
            handDealer.push_back(cards[spot]);          // add card to dealer hand
            spot++;                                     // next card
            handDealer.push_back(cards[spot]);          // add card to dealer hand
            spot++;                                     // next card

            // send two cards to p1
            card_p1 = {1, cards[spot]};                 // place card in buffer
            spot++;                                     // next card
            msgsnd(id_card1, &card_p1, 1, 0);           // send card to mq
            card_p1 = {1, cards[spot]};                 // place card in buffer
            spot++;                                     // next card
            msgsnd(id_card1, &card_p1, 1, 0);           // send card to mq

            // send two cards to p2
            card_p2 = {1, cards[spot]};                 // place card in buffer
            spot++;                                     // next card
            msgsnd(id_card2, &card_p2, 1, 0);           // send card to mq
            card_p2 = {1, cards[spot]};                 // place card in buffer
            spot++;                                     // next card
            msgsnd(id_card2, &card_p2, 1, 0);           // send card to mq

            /* Hit/Stand Response */
            // p1 sends hit or stand signal --> send card back until stand
            // p2 sends hit or stand signal --> send card back until stand
            bool statusP1, statusP2;                    // stores hit/stand signal from players
            msgrcv(id_hs1, &hs_p1, 1, 2, 0);            // hit/stand from p1
            statusP1 = hs_p1.hs;                        // copy attribute to status
            msgrcv(id_hs2, &hs_p2, 1, 2, 0);            // hit/stand from p2
            statusP2 = hs_p2.hs;                        // copy attribute to status

            // send cards to p1 until stand
            while(statusP1 == true)
            {
                card_p1 = {1, cards[spot]};             // place card in buffer
                spot++;                                 // next card
                msgsnd(id_card1, &card_p1, 1, 0);       // send card to mq                
                msgrcv(id_hs1, &hs_p1, 1, 2, 0);        // recieve updated status
                statusP1 = hs_p1.hs;                    // update status
            }
            // send cards to p2 until stand
            while(statusP2 == true)
            {
                card_p2 = {1, cards[spot]};             // place card in buffer
                spot++;                                 // next card
                msgsnd(id_card2, &card_p2, 1, 0);       // send card to mq                
                msgrcv(id_hs2, &hs_p2, 1, 2, 0);        // recieve updated status
                statusP2 = hs_p2.hs;                    // update status
            }

            /* Dealer Draws Cards */
            valDealer = handValue(handDealer);          // compute dealer's hand value
            statusDealer = dealer(valDealer);           // determine dealer's status
            while(statusDealer == true)                 // hit while status is true
            {
                handDealer.push_back(cards[spot]);      // add card to hand
                spot++;
                valDealer = handValue(handDealer);      // recompute hand value
                statusDealer = dealer(valDealer);       // recompute status of dealer
            }

            int valP1, valP2;                           // store final value of player hands
            msgrcv(id_hand1, &hand_p1, 4, 3, 0);        // final hand val of P1
            valP1 = hand_p1.hand;                       // store hand attribute
            msgrcv(id_hand2, &hand_p2, 4, 3, 0);        // final hand val of P2
            valP2 = hand_p2.hand;                       // store hand attribute

            /* Determine Wins */
            if(valDealer > 21)  // dealer busts
            {
                if(valP1 <= 21)                         // delaer busts, player1 <= 21, player1 wins
                    p1Wins++;
                if(valP2 <= 21)                         // dealer busts, player2 <= 21, player2 wins
                    p2Wins++;
                if(valP1 > 21 || valP2 > 21)            // either player busts, dealer wins
                    dealerWins++;
            }
            else    // dealer <= 21
            {
                if(valP1 <= 21 && valP1 > valDealer)        // player1 <= 21 and player > dealer, player1 wins
                    p1Wins++;
                if(valP2 <= 21 && valP2 > valDealer)        // player2 <= 21 and player > dealer, player2 wins
                    p2Wins++;
                if(valP1 < valDealer || valP2 < valDealer)  // dealer <= 21 and player < dealer, dealer wins
                    dealerWins++;
            }
        }     

        // 1000 games have finished 
        // display win stats for players and dealer
        cout << "\nMESSAGE QUEUE IMPLEMENTATION" << endl;
        cout << "Games:             1000" << endl;
        cout << "----------------------------------------------" << endl;
        cout << "Player One Wins:   " << p1Wins << " | Win Precentage: " << setprecision(4) << p1Wins/10.0 << "%" << endl;
        cout << "Player Two Wins:   " << p2Wins << " | Win Precentage: " << setprecision(4) << p2Wins/10.0 << "%" << endl;
        cout << "Dealer Wins:       " << dealerWins << " | Win Precentage: " << setprecision(4) <<dealerWins/10.0 << "%" << endl;

        // end the message queues
        msgctl(id_card1, IPC_RMID, NULL);   // remove card1
        msgctl(id_card2, IPC_RMID, NULL);   // remove card2
        msgctl(id_hs1, IPC_RMID, NULL);     // remove hs1
        msgctl(id_hs2, IPC_RMID, NULL);     // remove hs2
        msgctl(id_hand1, IPC_RMID, NULL);   // remove hand1
        msgctl(id_hand2, IPC_RMID, NULL);   // remove hand2

        return 0;
    }

    /* Worker Player Processes */
    else
    {
        /* Second fork() */
        pid_t pid2 = fork();

        if(pid2 < 0)    // fork failure
        {
            return 1;
        }
        
        /* Player 1 Process */
        else if(pid2 > 0)
        {
            // declare structs for recieving cards, sending hit/stand, sending hand value
            cardbuff card_p1;               // cards from dealer
            hsbuff hs_p1;                   // hs to dealer
            handbuff hand_p1;               // hand to dealer

            vector<char> handP1;            // p1 hand vector
            char c1, c2;                    // first two cards from dealer
            int valP1 = 0;                  // value of hand
            bool hitStand = false;          // hit or stand determination

            // iterations must be the same amount as parent for loop (1000)
            for(int p1 = 0; p1 < 1000; p1++)
            {
                handP1.clear();                             // clear hand
                msgrcv(id_card1, &card_p1, 1, 1, 0);        // read first card
                c1 = card_p1.card;                          // copy to c1
                msgrcv(id_card1, &card_p1, 1, 1, 0);        // read second card
                c2 = card_p1.card;                          // copy to c2

                handP1.push_back(c1);                       // add first card to vector
                handP1.push_back(c2);                       // add second card to vector

                valP1 = handValue(handP1);                  // calculate current total

                hitStand = playerOne(valP1);                // determine hit or stand
                hs_p1 = {2, hitStand};                      // set hit/stand buff attributes
                msgsnd(id_hs1, &hs_p1, 1, 0);               // send initial hit/stand

                while(hitStand == true)                     // while hit is true
                {
                    char temp;
                    msgrcv(id_card1, &card_p1, 1, 1, 0);    // recieve one more card
                    temp = card_p1.card;                    // store card attribute in temp
                    handP1.push_back(temp);                 // add temp to hand
                    valP1 = handValue(handP1);              // recompute hand value
                    hand_p1 = {3, valP1};                   // update hand buff
                    hitStand = playerOne(valP1);            // redetermine status
                    hs_p1 = {2, hitStand};                  // update hs buff
                    msgsnd(id_hs1, &hs_p1, 1, 0);           // send hs to dealer again
                }

                hand_p1 = {3, valP1};                       // update hand again before sending (in case player never hits)
                msgsnd(id_hand1, &hand_p1, 4, 0);           // send final hand value to dealer
            }
            exit(0);                                        // exit completed process
        }

        /* Player 2 Process */
        else
        {
            // declare structs for recieving cards, sending hit/stand, sending hand value
            cardbuff card_p2;                           // cards from dealer
            hsbuff hs_p2;                               // hs to dealer
            handbuff hand_p2;                           // hand to dealer

            vector<char> handP2;                        // p2 hand vector
            char c1, c2;                                // first two cards from dealer
            int valP2 = 0;                              // value of hand
            bool hitStand = false;                      // hit or stand determination

            // iterations must be the same amount as parent for loop (1000)
            for(int p2 = 0; p2 < 1000; p2++)
            {
                handP2.clear();                             // clear hand

                msgrcv(id_card2, &card_p2, 1, 1, 0);        // read first card
                c1 = card_p2.card;                          // copy to c1
                msgrcv(id_card2, &card_p2, 1, 1, 0);        // read second card
                c2 = card_p2.card;                          // copy to c2

                handP2.push_back(c1);                       // add first card to vector
                handP2.push_back(c2);                       // add second card to vector

                valP2 = handValue(handP2);                  // calculate current total

                hitStand = playerTwo(valP2);                // determine hit or stand
                hs_p2 = {2, hitStand};                      // set hit/stand buff attributes
                msgsnd(id_hs2, &hs_p2, 1, 0);               // send initial hit/stand

                while(hitStand == true)                     // while hit is true
                {
                    char temp;
                    msgrcv(id_card2, &card_p2, 1, 1, 0);    // recieve one more card
                    temp = card_p2.card;                    // store card attribute in temp
                    handP2.push_back(temp);                 // add temp to hand
                    valP2 = handValue(handP2);              // recompute hand value
                    hand_p2 = {3, valP2};                   // update hand buff
                    hitStand = playerTwo(valP2);            // redetermine status
                    hs_p2 = {2, hitStand};                  // update hs buff
                    msgsnd(id_hs2, &hs_p2, 1, 0);           // send hs to dealer again
                }

                hand_p2 = {3, valP2};                       // update hand again before sending (in case player never hits)
                msgsnd(id_hand2, &hand_p2, 4, 0);           // send final hand value to dealer
            }
            exit(0);                                        // exit completed process
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
* Description: Computes the integer value of the hand vector argument.
*              Aces are treated specially and are dependent on the value of the
*              other cards in the hand, and how many other aces are in the hand.
*
* Parameters:
*   hand        I/P     vector<char>    Vector of chars containing cards in current hand 
*   handValue   O/P     int             Integer value of hand computed from vector hand
***************************************************************************/
int handValue(vector<char> hand)
{
    int sum = 0;    // running sum of hand
    int aces = count(hand.begin(), hand.end(), 'A');    // occurences of 'A' in vector

    // if aces exist in hand
    if(aces > 0)
    {
        // iterate through hand and sum non-ace values
        for(auto c: hand)
        {
            if(c != 'A')                // card must not be an ace
            {
                if(c == 'T')            // 'T' equivalent to 10,J,Q,K
                    sum += 10;          // add to sum
                else                    // treat all other cards as face value
                {
                    int val = c - '0';  // convert char to int
                    sum += val;         // add to sum
                }
            }
        }

        // remaining cards are aces
        // depending on how many aces are present
        // the values will be treated as 1 for each ace accordingly
        switch (aces)
        {
        case 1: /* One Ace: 1 or 11 */
            if(sum <= 10)               // if existing sum is able to fit 11 (blackjack if sum = 10!)
                sum += 11;
            else                        // else treat aces as 1
                sum += 1;
            break;
        case 2: /* Two Ace: 2 or 12 */
            if(sum <= 9)                // existing sum able to fit 12 (11, 1)
                sum += 12;
            else                        // else treat aces as 1 (1, 1)
                sum += 2;
            break;
        case 3: /* Three Ace: 3 or 13 */
            if(sum <= 8)                // existing sum able to fit 13 (11, 1, 1)
                sum += 13;
            else                        // else treat aces as 1 (1, 1, 1)
                sum += 3;
            break;
        case 4: /* Four Ace: 4 or 14 */
            if(sum <= 7)                // existing sum able to fit 14 (11, 1, 1, 1)
                sum += 14;
            else                        // else treat aces as 1 (1, 1, 1, 1)
                sum += 4;
            break;
        default:                        // default case will not occur
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
            if(c == 'T')                // 'T' equivalent to 10,J,Q,K
                sum += 10;              // add to sum
            else                        // treat all other cards as face value
            {
                int val = c - '0';      // convert char to int
                sum += val;             // add to sum
            }
        }
    }
    return sum;                         // return sum of hand
}
