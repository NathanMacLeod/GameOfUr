#define OLC_PGE_APPLICATION
#define OLC_PGEX_SOUND

#include "olcPixelGameEngine.h"
#include "olcPGEX_Sound.h"
#include "cstdio"
#include <iostream>    
#include <stdio.h>
#include <chrono>
#include <cstdlib>
#include <time.h>
#include "Timer.h"
#include <cstring>

#define NO_SELECT -2
#define UNPLACED -1

class Ur : public olc::PixelGameEngine
{
public:
	Ur()
	{
		sAppName = "Ur";	
	}

public:

	bool OnUserCreate();

	bool OnUserUpdate(float fElapsedTime);

private:

	olc::Sprite* board;
	olc::Sprite* dieSprites[6];
	int dieRollSFX;
	int placePieceSFX;
	int winSFX;

	Timer timer;

	int dieStates[4];

	int dieWidth;
	int dieHeight;
	int dieY;
	int dieX;
	int dieValue;
	int boardX;
	int boardY;
	int tileStartX = 10;
	int tileStartY = 10;
	int tileWidth = 81;
	int tileHeight = 81;
	int gap = 8;
	float rollTime = 1.5;
	float ai_delay = 0.5;
	float pieceMoveTime = 0.35;
	int rerollCount = 12;
	int pieceYGap = 30;
	int pieceGap = 6;
	int pieceRadius = 23;
	int text_size = 7;
	int n_pieces = 7;

	enum TurnState {RollDie, MidRoll, SelectingMove, PieceMoving, WhiteWin, BlackWin};
	bool whiteTurn = true;
	bool black_AI = true;
	TurnState turnState;

	int selectedIndex; 
	int pieceSource;
	int pieceCurrent;
	int pieceTarget;
	int movingPieceSourceX;
	int movingPieceSourceY;
	int movingPieceTargetX;
	int movingPieceTargetY;
	int movingPieceX;
	int movingPieceY;
	
	const static int runSize = 14;
	bool blackPieces[runSize];
	bool whitePieces[runSize];
	int completedLineX;
	int nWhiteUnplaced;
	int nWhiteCompleted;
	int nBlackUnplaced;
	int nBlackCompleted;

	void resetGame();
	void getIndexCoords(int i, bool white, int* xOut, int* yOut);
	void rerollDie();
	int countDie();
	void drawBoard();
	bool checkMovePossible(int index, bool whiteTurn);
	bool checkMoveExists(bool whiteTurn);
	bool indexCapturable(int index, bool whiteTurn);
	int getMovePrio(int index, bool whiteTurn);
	int getBestMove(bool whiteTurn);
	int getPieceDangerValue(int index, bool whiteTurn);
	bool moveBetterThan(int m1, int m2, bool whiteTurn);
	bool isLotusIndex(int i);
	void gameUpdate(float timePassed);
	void drawPiece(int x, int y, int radius, bool white);
	int getClickItem(int x, int y, bool white);
};

bool Ur::OnUserCreate() {

	srand(time(NULL));

	olc::ResourcePack* pack = new olc::ResourcePack();
	pack->LoadPack("./FilePack.dat", "GAME OF UR");

	board = new olc::Sprite("images/board.png", pack);

	dieSprites[0] = new olc::Sprite("images/die1.png", pack);
	dieSprites[1] = new olc::Sprite("images/die2.png", pack);
	dieSprites[2] = new olc::Sprite("images/die3.png", pack);
	dieSprites[3] = new olc::Sprite("images/die4.png", pack);
	dieSprites[4] = new olc::Sprite("images/die5.png", pack);
	dieSprites[5] = new olc::Sprite("images/die6.png", pack);

	olc::SOUND::InitialiseAudio();

	dieRollSFX = olc::SOUND::LoadAudioSample("sfx/dieRoll.wav", pack);
	placePieceSFX = olc::SOUND::LoadAudioSample("sfx/placePiece.wav", pack);
	winSFX = olc::SOUND::LoadAudioSample("sfx/win.wav", pack);

	//for generating the resource pack from the original files
	if (false) {
		pack->AddFile("images/board.png");
		pack->AddFile("images/die1.png");
		pack->AddFile("images/die2.png");
		pack->AddFile("images/die3.png");
		pack->AddFile("images/die4.png");
		pack->AddFile("images/die5.png");
		pack->AddFile("images/die6.png");

		pack->AddFile("sfx/dieRoll.wav");
		pack->AddFile("sfx/placePiece.wav");
		pack->AddFile("sfx/win.wav");
		pack->SavePack("FilePack.dat", "GAME OF UR");
	}

	resetGame();

	return true;
}

void Ur::resetGame() {
	for (int i = 0; i < 4; i++) {
		dieStates[i] = i;
	}

	nWhiteUnplaced = n_pieces;
	nBlackUnplaced = n_pieces;
	nWhiteCompleted = 0;
	nBlackCompleted = 0;

	movingPieceX = -1;
	movingPieceY = -1;

	dieWidth = dieSprites[0]->width;
	dieHeight = dieSprites[0]->height;
	dieY = ScreenHeight() * 4 / 5 - dieHeight / 2;
	dieX = (ScreenWidth() - dieWidth * 4) / 2;

	boardX = (ScreenWidth() - board->width) / 2;
	boardY = ScreenHeight() * 5 / 14 - board->height / 2;
	completedLineX = boardX + board->width * 3 / 4;

	for (int i = 0; i < runSize; i++) {
		whitePieces[i] = false;
		blackPieces[i] = false;
	}

	turnState = RollDie;
	whiteTurn = true;
}

int Ur::getBestMove(bool whiteTurn) {
	int best = NO_SELECT;
	for (int i = UNPLACED; i < runSize; i++) {
		if (checkMovePossible(i, whiteTurn) && moveBetterThan(i, best, whiteTurn)) {
			best = i;
		}
	}
	return best;
}

int Ur::getPieceDangerValue(int index, bool whiteTurn) {
	bool* enemyPieces = whiteTurn ? blackPieces : whitePieces;
	
	if (index <= 3 || index >= 12 || isLotusIndex(index)) {
		return 0; //safe space
	}

	int oneAway = enemyPieces[index - 1] ? 1 : 0;
	int twoAway = enemyPieces[index - 2] ? 1 : 0;
	int threeAway = enemyPieces[index - 3] ? 1 : 0;
	int fourAway = enemyPieces[index - 4] ? 1 : 0;

	return 4 * oneAway + 6 * twoAway + 4 * threeAway + fourAway; //numerator out of 16 of being captured
}

int Ur::getMovePrio(int index, bool whiteTurn) {
	int landingIndex = index + dieValue;
	if (indexCapturable(landingIndex, whiteTurn)) {
		return 0;
	}
	else if (isLotusIndex(landingIndex)) {
		return 1;
	}
	else if (getPieceDangerValue(index, whiteTurn) > 1 && getPieceDangerValue(index, whiteTurn) - getPieceDangerValue(index + dieValue, whiteTurn) >= 0) {
		return 2;
	}
	else if (index + dieValue == runSize) {
		return 3;
	}
	else if (index == -1) {
		return 4;
	}
	else {
		return 5;
	}
}

//true => m1 is better than m2, false => m2 is better than m1
bool Ur::moveBetterThan(int m1, int m2, bool whiteTurn) {
	if (m2 == NO_SELECT) {
		return true;
	}

	int m1Prio = getMovePrio(m1, whiteTurn);
	int m2Prio = getMovePrio(m2, whiteTurn);

	if (m1Prio < m2Prio) {
		return true;
	}
	else if (m2Prio < m1Prio) {
		return false;
	}
	else {
		//tie breaker- depends on prio value
		switch (m1Prio) {
		case 0:
			return m1 > m2;
			break;
		case 1:
			return m1 + dieValue == 7 || (m2 + dieValue != 7 && m1 > m2);
			break;
		case 2:
		case 5:
			if (m1 == 7) {
				return false;
			}
			else if (m2 == 7) {
				return true;
			}
			
			int d1 = getPieceDangerValue(m1, whiteTurn) - getPieceDangerValue(m1 + dieValue, whiteTurn);
			int d2 = getPieceDangerValue(m2, whiteTurn) - getPieceDangerValue(m2 + dieValue, whiteTurn);
			if (d1 == d2) {
				return m1 > m2;
			}
			else {
				return d1 > d2;
			}
			break;
		//case 3, case 4 impossible for both
		}
	}
}

bool Ur::isLotusIndex(int index) {
	return index == 3 || index == 7 || index == 13;
}

bool Ur::indexCapturable(int target, bool whiteTurn) {
	bool* enemyPieces = whiteTurn ? blackPieces : whitePieces;
	return target > 3 && target < 12 && enemyPieces[target] && !isLotusIndex(target);
}

bool Ur::checkMovePossible(int index, bool whiteTurn) {
	bool* pieces = whiteTurn ? whitePieces : blackPieces;
	if (dieValue == 0 || (index != -1 && !pieces[index])) {
		return false;
	}
	int target = index + dieValue;
	if (index == UNPLACED && (whiteTurn ? nWhiteUnplaced : nBlackUnplaced) <= 0) {
		return false;
	}
	bool* enemyPieces = whiteTurn ? blackPieces : whitePieces;

	if (target > runSize) {
		return false;
	}
	else if (target == runSize) {
		return true;
	}
	return !(pieces[target] || (target > 3 && target < 12 && enemyPieces[target] && isLotusIndex(target)));
}

bool Ur::checkMoveExists(bool whiteTurn) {
	bool* pieces = whiteTurn ? whitePieces : blackPieces;

	for (int i = -1; i < runSize; i++) {
		if (checkMovePossible(i, whiteTurn)) {
			return true;
		}
	}

	return false;
}

bool Ur::OnUserUpdate(float fElapsedTime) {

	gameUpdate(fElapsedTime);
	drawBoard();

	return true;
}

void Ur::gameUpdate(float timePassed) {
	bool* pieces = whiteTurn ? whitePieces : blackPieces;
	bool* enemyPieces = whiteTurn ? blackPieces : whitePieces;

	switch (turnState) {
	case RollDie:
	{
		if (GetMouse(0).bPressed || (!whiteTurn && black_AI && timer.update(timePassed))) {
			turnState = MidRoll;
			timer.setTimer(rollTime);
			olc::SOUND::PlaySample(dieRollSFX);
		}
		break;
	}
	case MidRoll:
	{
		int n1 = (int)(timer.getCurrTime() * rerollCount / rollTime);
		bool diceDone = timer.update(timePassed);
		int n2 = (int)(timer.getCurrTime() * rerollCount / rollTime);
		if (n2 != n1) {
			rerollDie();
		}

		if (diceDone) {
			
			dieValue = countDie();
			if (checkMoveExists(whiteTurn)) {
				selectedIndex = NO_SELECT;
				turnState = SelectingMove;
			}
			else {
				turnState = RollDie;
				timer.setTimer(ai_delay);
				whiteTurn = !whiteTurn;
			}
		}
		break;
	}
	case SelectingMove:
	{
		if (!whiteTurn && black_AI) {
			pieceSource = getBestMove(whiteTurn);
			pieceCurrent = pieceSource;
			pieceTarget = pieceSource + dieValue;

			if (pieceSource == -1) {
				if (whiteTurn) {
					nWhiteUnplaced--;
				}
				else {
					nBlackUnplaced--;
				}
			}
			else {
				bool* pieces = whiteTurn ? whitePieces : blackPieces;
				pieces[pieceSource] = false;
			}

			turnState = PieceMoving;
			getIndexCoords(pieceSource, whiteTurn, &movingPieceX, &movingPieceY);
			timer.setTimer(0);
		}
		else if (GetMouse(0).bPressed) {
			int selected = getClickItem(GetMouseX(), GetMouseY(), whiteTurn);
			bool moveCompleted = false;

			//player trying to input a move, check is valid
			if(selectedIndex != NO_SELECT) {
				if (selectedIndex == UNPLACED && checkMovePossible(selectedIndex, whiteTurn)) {
					if (selected == dieValue - 1) {
						moveCompleted = true;
						if (whiteTurn) {
							nWhiteUnplaced--;
						}
						else {
							nBlackUnplaced--;
						}
					}
					else {
						selectedIndex = NO_SELECT; //move invalid, undo selection
					}
				}
				else if (selected == selectedIndex + dieValue && checkMovePossible(selectedIndex, whiteTurn)) {
					moveCompleted = true;
					pieces[selectedIndex] = false;
				}
				else {
					selectedIndex = NO_SELECT;
				}
			}

			if (selectedIndex == NO_SELECT) {
				//check if there is actually something to select at this index
				if (selected == UNPLACED) {
					//only possible to select if pieces are left, so this always works
					selectedIndex = selected;
				}
				else if (selected >= 0 && selected < runSize && pieces[selected]) {
					//check piece is at this index
					selectedIndex = selected;
				}
			}

			if (moveCompleted) {
				turnState = PieceMoving;
				pieceSource = selectedIndex;
				pieceCurrent = pieceSource;
				pieceTarget = selected;
				getIndexCoords(pieceSource, whiteTurn, &movingPieceX, &movingPieceY);
				timer.setTimer(0);
			}
		}
		break;
	}
	case PieceMoving:
	{
		if (timer.update(timePassed)) {
			if (pieceCurrent != pieceSource) {
				olc::SOUND::PlaySample(placePieceSFX);
			}
			if (pieceCurrent != pieceTarget) {

				timer.setTimer(pieceMoveTime);
				getIndexCoords(pieceCurrent, whiteTurn, &movingPieceSourceX, &movingPieceSourceY);


				if (pieceCurrent == UNPLACED) {
					pieceCurrent = pieceTarget; //marks that moving is done after timer resets
					getIndexCoords(pieceTarget, whiteTurn, &movingPieceTargetX, &movingPieceTargetY);
				}
				else {
					getIndexCoords(pieceCurrent + 1, whiteTurn, &movingPieceTargetX, &movingPieceTargetY);
					pieceCurrent++; //advances piece accross board to track as it moves
				}
			}
			else if (pieceTarget == runSize) {
				//todo
				if (whiteTurn) {
					nWhiteCompleted++;
				}
				else {
					nBlackCompleted++;
				}

				movingPieceX = -1;
				movingPieceY = -1;
				if (nBlackCompleted == n_pieces) {
					timer.setTimer(5);
					turnState = BlackWin;
				}
				else if (nWhiteCompleted == n_pieces) {
					timer.setTimer(5);
					olc::SOUND::PlaySample(winSFX);
					turnState = WhiteWin;
				}
				else {
					turnState = RollDie;
					whiteTurn = !whiteTurn;
				}
			}
			else {
				//todo: capturing
				if (indexCapturable(pieceTarget, whiteTurn)) {
					enemyPieces[pieceTarget] = false;
					if (whiteTurn) {
						nBlackUnplaced++;
					}
					else {
						nWhiteUnplaced++;
					}
				}
				pieces[pieceTarget] = true;
				movingPieceX = -1;
				movingPieceY = -1;
				turnState = RollDie;

				if (!isLotusIndex(pieceTarget)) {
					whiteTurn = !whiteTurn;
					timer.setTimer(ai_delay);
				}
			}
		}
		else {

			float t = pieceMoveTime - timer.getCurrTime();
			int dx = movingPieceTargetX - movingPieceSourceX;
			int dy = movingPieceTargetY - movingPieceSourceY;

			movingPieceX = movingPieceSourceX + t * dx / pieceMoveTime - 0.5 * dx * sin(3.14159 * t / pieceMoveTime) / 3.14159;
			movingPieceY = movingPieceSourceY + t * dy / pieceMoveTime - 0.5 * dy * sin(3.14159 * t / pieceMoveTime) / 3.14159;
		}
		break;
	}
	case WhiteWin:
	case BlackWin:
	{
		if (timer.update(timePassed)) {
			resetGame();
		}
	}
	}
}

void Ur::drawPiece(int x, int y, int radius, bool white) {
	olc::Pixel prim = white ? olc::Pixel(230, 230, 230) : olc::VERY_DARK_GREY;
	olc::Pixel sec = white ? olc::VERY_DARK_GREY : olc::Pixel(230, 230, 230);

    FillCircle(x, y, radius, olc::Pixel(20, 20, 20));
	FillCircle(x, y, radius * 9/10, prim);
	
	int scale = 9;
	int spacing = radius * 3 / scale;
	FillCircle(x, y, radius / scale, sec);
	FillCircle(x - spacing, y - spacing , radius / scale, sec);
	FillCircle(x + spacing, y - spacing, radius / scale, sec);
	FillCircle(x - spacing, y + spacing, radius / scale, sec);
	FillCircle(x + spacing, y + spacing, radius / scale, sec);
}

void Ur::drawBoard() {
	SetPixelMode(olc::Pixel::NORMAL);
	Clear(olc::VERY_DARK_BLUE);
	SetPixelMode(olc::Pixel::ALPHA);
	DrawSprite(boardX, boardY, board);

	for (int i = 0; i < nWhiteUnplaced; i++) {
		int y = boardY - pieceYGap - pieceRadius;
		int x = boardX + i * (2 * pieceRadius + pieceGap);

		drawPiece(x, y, pieceRadius, true);
	}

	for (int i = 0; i < nWhiteCompleted; i++) {
		int y = boardY - pieceYGap - pieceRadius;
		int x = completedLineX + i * (2 * pieceRadius + pieceGap);

		drawPiece(x, y, pieceRadius, true);
	}

	for (int i = 0; i < nBlackUnplaced; i++) {
		int y = boardY + board->height + pieceYGap + pieceRadius;
		int x = boardX + i * (2 * pieceRadius + pieceGap);

		drawPiece(x, y, pieceRadius, false);
	}

	for (int i = 0; i < nBlackCompleted; i++) {
		int y = boardY + board->height + pieceYGap + pieceRadius;
		int x = completedLineX + i * (2 * pieceRadius + pieceGap);

		drawPiece(x, y, pieceRadius, false);
	}

	for (int i = 0; i < runSize; i++) {

		if (whitePieces[i]) {
			int x, y;
			getIndexCoords(i, true, &x, &y);
			drawPiece(x, y, pieceRadius, true);
		}

		if (blackPieces[i]) {
			int x, y;
			getIndexCoords(i, false, &x, &y);
			drawPiece(x, y, pieceRadius, false);
		}
	}

	if (movingPieceX != -1) {
		drawPiece(movingPieceX, movingPieceY, pieceRadius, whiteTurn);
	}

	if (turnState == SelectingMove && selectedIndex != NO_SELECT && checkMovePossible(selectedIndex, whiteTurn)) {
		int x, y;
		getIndexCoords(selectedIndex + dieValue, whiteTurn, &x, &y);
		FillCircle(x, y, pieceRadius, olc::Pixel(200, 0, 0, 100));
	}
	if (turnState == WhiteWin || turnState == BlackWin) {
		const char* message = (turnState == WhiteWin) ? "White Wins" : "Black Wins";
		int messageX = (ScreenWidth() - 8 * text_size * strlen(message)) / 2;
		int messageY = boardY + (board->height - 8 * text_size) / 2;
		DrawString(messageX, messageY, message, olc::BLUE, text_size);
	}

	for (int i = 0; i < 4; i++) {
		int x = dieX + i * dieWidth;
		
		DrawSprite(x, dieY, dieSprites[dieStates[i]]);
	}
}

void Ur::getIndexCoords(int i, bool white, int* xOut, int* yOut) {

	if (i == UNPLACED) {
		*xOut = boardX + (white ? nWhiteUnplaced : nBlackUnplaced) * (2 * pieceRadius + pieceGap);
		*yOut = white ? boardY - pieceYGap - pieceRadius : boardY + board->height + pieceYGap + pieceRadius;
		return;
	}

	int tileX, tileY;
	if (i <= 3 || i >= 12) {
		tileY = (white? 0 : 2);
	}
	else {
		tileY = 1;
	}

	if (i <= 3) {
		tileX = 3 - i;
	}
	else if (i >= 12) {
		tileX = 7 - (i - 12);
	}
	else {
		tileX = i - 4;
	}

	*xOut = boardX + tileStartX + tileX * (tileWidth + gap) + tileWidth / 2;
	*yOut = boardY + tileStartY + tileY * (tileHeight + gap) + tileHeight / 2;
}

int Ur::getClickItem(int x, int y, bool white) {
	//checking if clicked on new piece to place
	int unplacedX = boardX - pieceRadius;
	int unplacedY = white? boardY - pieceYGap - pieceRadius * 2 : boardY + board->height + pieceYGap;
	int nUnplaced = white ? nWhiteUnplaced : nBlackUnplaced;
	int unplacedWidth = (nUnplaced == 0)? 0 : nUnplaced * (2 * pieceRadius + pieceGap) - pieceGap;

	
	if (x > unplacedX && x < unplacedX + unplacedWidth && y > unplacedY && y < unplacedY + 2 * pieceRadius) {
		return UNPLACED;
	}

	//Checking if a board tile was clicked
	int relX = x - boardX - tileStartX;
	int relY = y - boardY - tileStartY;
	int tileX = relX / (tileWidth + gap);
	int tileY = relY / (tileWidth + gap);

	if (relX % (tileWidth + gap) >= tileWidth || relY % (tileHeight + gap) >= tileHeight) {
		return NO_SELECT; //clicking gap
	}

	if (tileY < 0 || tileY >= 3 || tileX < 0 || tileX >= 8) {
		return NO_SELECT; //out of bounds
	}

	if ((white && tileY == 0) || (!white && tileY == 2)) {
		if (tileX <= 3) {
			return 3 - tileX;
		}
		else if (tileX >= 5) {
			return 14 - (tileX - 5);
		}
		else {
			return NO_SELECT;
		}
	}
	else if (tileY != 1) {
		return NO_SELECT;
	}
	else {
		return tileX + 4;
	}
}

void Ur::rerollDie() {
	for (int i = 0; i < 4; i++) {
		dieStates[i] = rand() % 6;
	}
}

int Ur::countDie() {
	int c = 0;
	for (int i = 0; i < 4; i++) {
		if (dieStates[i] < 3) {
			c++;
		}
	}
	return c;
}

int main() {
	Ur game;
	if (game.Construct(1260, 640, 1, 1))
		game.Start();
}