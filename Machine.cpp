/***************************************************************************
 *   Copyright (C) 2016 by Herbert Roider                                  *
 *   herbert.roider@utanet.at                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *

 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


/* 
 * File:   Machine.cpp
 * Author: Herbert Roider <herbert@roider.at>
 * 
 * Created on 13. Februar 2016, 08:42
 */

#include "Machine.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>
#include <softPwm.h>
#include "Geometry.h"

// value for the servo
#define SERVOUP        13
// value for the servo
#define SERVODOWN      8

// step wiringpi pin 0
#define LEFT_STEPPER01 0
// direction  wiring pin 1
#define LEFT_STEPPER02 7


// step  wiring pin 3
#define RIGHT_STEPPER01  3
// direction   wiring pin 4
#define RIGHT_STEPPER02  4

// Wiring pi for the servo pin 1 
#define Z_SERVO 1

Machine::Machine() {
    this->BaseLength = 690.0;
    this->X0 = 135;
    this->Y0 = -40 - 650;

    this->StepsPermm = 40.0; // 51   //2000.0 / 98.0;

    this->currentX = 0.0;
    this->currentY = 0.0;

    if (wiringPiSetup() == -1) {
        std::cout << "Could not run wiringPiSetup, require root privileges!\n";
        exit(1);
    }

    softPwmCreate(Z_SERVO, SERVOUP, 200);

    this->penDownUp(false);


    pinMode(LEFT_STEPPER01, OUTPUT);
    pinMode(LEFT_STEPPER02, OUTPUT);

    digitalWrite(LEFT_STEPPER01, 0);
    digitalWrite(LEFT_STEPPER02, 0);

    pinMode(RIGHT_STEPPER01, OUTPUT);
    pinMode(RIGHT_STEPPER02, OUTPUT);

    digitalWrite(RIGHT_STEPPER01, 0);
    digitalWrite(RIGHT_STEPPER02, 0);



    this->CordLengthLeft = sqrt(this->X0 * this->X0 + this->Y0 * this->Y0) * this->StepsPermm;
    this->CordLengthRight = sqrt((this->BaseLength - this->X0) * (this->BaseLength - this->X0) + this->Y0 * this->Y0) * this->StepsPermm;



    std::cout << "X0=" << this->X0 << ", Y0=" << this->Y0 << ", cord left=" << this->CordLengthLeft << ", cord right=" << this->CordLengthRight << ", Steps per mm=" << this->StepsPermm << std::endl;

}

Machine::Machine(const Machine& orig) {
}

Machine::~Machine() {
}

/**
 * moves the left motor 1 step
 *
 * @param direction -1 or +1
 * @param count    count of steps
 * @param time     duration per step [microsecond]
 */
void Machine::MakeStepLeft(int direction, int count = 1, unsigned int time = 100) {

    if (direction > 0) {
        digitalWrite(LEFT_STEPPER02, 1);
    } else {
        digitalWrite(LEFT_STEPPER02, 0);
    }
    for (int i = 0; i < count; i++) {
        digitalWrite(LEFT_STEPPER01, 1);
        ::usleep(time/2);
        digitalWrite(LEFT_STEPPER01, 0);
        ::usleep(time/2);
        this->CordLengthLeft += direction;
    }

}

/**
 *  move the right motor 1 step 
 * 
 * @param direction -1 or +1
 * @param count    count of steps
 * @param time     duration per step [microsecond]
*/
void Machine::MakeStepRight(int direction, int count = 1, unsigned int time = 100) {

    if (direction > 0) {
        digitalWrite(RIGHT_STEPPER02, 0);
    } else {
        digitalWrite(RIGHT_STEPPER02, 1);
    }
    for (int i = 0; i < count; i++) {
        digitalWrite(RIGHT_STEPPER01, 1);
        ::usleep(time/2);
        digitalWrite(RIGHT_STEPPER01, 0);
        ::usleep(time/2);
        this->CordLengthRight += direction;
    }

}


/**
 * moves max. 1000 steps per Motor in direction to the given point.
 * @param X
 * @param Y
 * @param time   time per step  [microsecond]
 * @return true if position is reached 
 */
bool Machine::moveShortDist(double X, double Y, unsigned int time) {
    double newCordLengthLeft;
    double newCordLengthRight;
    int dirRight;
    int dirLeft;
    int max_steps = 1000;
    bool ready = true;

    X += this->X0;
    Y += this->Y0;


    newCordLengthLeft = this->StepsPermm * sqrt((X * X) + (Y * Y));
    newCordLengthRight = this->StepsPermm * sqrt(((BaseLength - X) * (BaseLength - X)) + (Y * Y));

    double old_CordLengthLeft = (double) this->CordLengthLeft;
    double old_CordLengthRight = (double) this->CordLengthRight;

    double stepsLeft = newCordLengthLeft - (double) CordLengthLeft;
    double stepsRight = newCordLengthRight - (double) CordLengthRight;


    // set the directions for the steppers
    if (stepsLeft > 0) {
        dirLeft = 1;
    } else {
        dirLeft = -1;

    }
    if (stepsRight > 0) {
        dirRight = 1;
    } else {
        dirRight = -1;

    }

    stepsLeft = fabs(stepsLeft);
    stepsRight = fabs(stepsRight);



    //std::cout << "steps left=" << stepsLeft << ", right=" << stepsRight << std::endl;

    double fact;
    
    if (stepsLeft < 0.5 && stepsRight < 0.5) {
        return ready;
        
    } else if (stepsRight < 0.5) {

        if (stepsLeft > max_steps) {
            stepsLeft = max_steps;
            ready = false;

        }
        this->MakeStepLeft(dirLeft, stepsLeft);

    } else if (stepsLeft < 0.5) {

        if (stepsRight > max_steps) {
            stepsRight = max_steps;
            ready = false;

        }
        this->MakeStepRight(dirRight, stepsRight, time);



    } else if (stepsLeft >= stepsRight) {
        fact = stepsLeft / stepsRight;
        if (stepsLeft > max_steps) {
            stepsLeft = max_steps;
            stepsRight = stepsLeft / fact;
            ready = false;
        }
        for (double r = 0.0; r < stepsRight; r++) {
            this->MakeStepRight(dirRight, 1, time);
            this->MakeStepLeft(dirLeft, (int) (r * fact - fabs(old_CordLengthLeft - (double) this->CordLengthLeft)), time);
        }

    } else if (stepsRight > stepsLeft) {
        fact = stepsRight / stepsLeft;
        if (stepsRight > max_steps) {
            stepsRight = max_steps;
            stepsLeft = stepsRight / fact;
            ready = false;
        }
        for (double l = 0.0; l < stepsLeft; l++) {
            this->MakeStepLeft(dirLeft, 1, time);
            this->MakeStepRight(dirRight, (int) (l * fact - fabs(old_CordLengthRight - (double) this->CordLengthRight)), time);
        }

    } else {
        std::cout << "Error  4567" << std::endl;
        exit(1);

    }




    /*
    std::cout << "soll pos: l=" << (int)newCordLengthLeft << ", r=" << (int)newCordLengthRight << " ist pos: l=" << this->CordLengthLeft << ", r=" << this->CordLengthRight 
            << " delta l=" << (int)stepsLeft << " delta r=" << (int)stepsRight << std::endl;
     
     
     */

    return ready;


}


/**
 * moves the pen to a point in a straight way
 * @param X
 * @param Y
 * @param F Feed in mm/min
 * @return 
 */
int Machine::MoveToPoint(double X, double Y, double F) {    
    unsigned int time = 0;
    if(F <=0.0){
        F = 100000; // big number for rapid speed
    }
    F *= 1.2; // correction factor, the value fits better
    time = 1000000 * 60/(this->StepsPermm*F);
    if(time < 100){
        time = 100;
    }
    std::cout << "time=" << time << std::endl;
    
    Point p2(X,Y);
    p2 = movePoint(p2, -this->currentX, -this->currentY);
    p2 = toPolar(p2);
    Point pi(p2); // interpolatios point
    pi.x = 1.0; // steps of one millimeter
    Point pi_cart;
    
    while(pi.x < p2.x){
        pi_cart = movePoint(toCartesian(pi),this->currentX, this->currentY);       
        this->moveShortDist(pi_cart.x, pi_cart.y, time);
        pi.x += 1.0; // steps of one millimeter
        
    }
    this->moveShortDist(X, Y, time);
            
    this->currentX = X;
    this->currentY = Y;
    return 0;
}

/**
 * raise and down the pen
 * @param down
 */
void Machine::penDownUp(bool down) {
    if (down) {
        softPwmWrite(Z_SERVO, SERVODOWN);
        usleep(500000);
        softPwmWrite(Z_SERVO, 0);

    } else {
        softPwmWrite(Z_SERVO, SERVOUP);
        usleep(500000);
        softPwmWrite(Z_SERVO, 0);


    }

}