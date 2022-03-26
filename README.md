# Blackjack - Multiprocessing in UNIX with C++

This project is a demonstration of a multiprocessing manager/worker program that implements the game Blackjack (21). The AIM of this project is to test different blackjack strategies in a multiplayer game using Multiprocessing in a UNIX environment (and hopefully learn something along the way).

The dealer is represented by the manager processes, and the two players represented by the worker processes. There exist two files in this repository. Each implement interprocess communication (IPC) between the actors in a different way:

 - blackjack_mq.cpp : IPC is done through the use of a messaging queue.
 - blackjack_pipes.cpp	: IPC is done through the use of pipes.

Please refer to the below section "Game Details" for details on how the game works, and what the output analyzes. For specific details on what each function or struct does, please refer to the code itself. Function documentation is in-line for this project.

# Execution Instructions
Before Attempting to compile and run code. Please ensure you are operating in a UNIX-like environment. Please note that running the programs in a containerized environment (such as Cygwin on Windows) may produce different results than running on Ubuntu or MacOS, due to how the IPC is implemented. You may compile using either g++ or gcc, however I will be using g++ since these are implemented in C++.

 - cd into the working directory
 - run one of the following commands to compile:
	 - g++ blackjack_mq.ccp -o mq
	 - g++ blackjack_pipes.cpp -o pipes
- execute the program:
	- ./mq
	- ./pipes

# Game Details

In this program, the deck of cards is represented by a char array of 52 cards. <![endif]--> Depending on the player’s current hand, Aces can be treated as either 1 or 11. 10, J, Q, K are represented by the char ‘T’, aces are represented by the char ‘A’, and all other cards are represented by their face value in character form. The function handValue() determines integer value of the passed hand. Hands are passed into the function as a vector of chars. Each iteration, the deck is reshuffled using the random_shuffle() function.

A simulation of 1000 games are played and the percent win rate is calculated and displayed for the Dealer, and both Player processes.

### Player Strategies

 - Player one will hit while it’s hand is less than 15 and stand when greater or equal to 15.
 - Player two will hit while it’s hand is less than 18 and stand when greater or equal to 18.
 - For both players, aces are treated as 11 when alone, 1s if the total of non-aces is 20, or a combination of 11 and 1s if multiple aces are present.

### Game Rules

 - The deck contains 52 shuffled cards each round.
 - 10, Jack, Queen, King have a value of 10.
 - Aces have a value of 1 or 11 depending on the current hand (see G and H).
 - The dealer deals 2 cards to itself and each player at the start of the game.
 - Each player will hit or stand depending on its specific strategy, until its stand condition is met or it busts.
 - Wins are determined after every player and the dealer has made its turn.

### Player Win Conditions

 - Dealer busts, and player hand is <= 21.
 - Dealer hand <= 21, and player hand > dealer hand.

### Dealer Win Conditions

 - Dealer may bust and still win if at least one player busts.
 - Dealer hand <= 21 and dealer hand > at least one player hand.