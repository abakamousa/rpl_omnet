/* -*- mode:c++ -*- ********************************************************
 * file:        MobilityBase.cc
 *
 * author:      Daniel Willkomm, Andras Varga, Zoltan Bojthe
 *
 * copyright:   (C) 2004 Telecommunication Networks Group (TKN) at
 *              Technische Universitaet Berlin, Germany.
 *
 *              (C) 2005 Andras Varga
 *              (C) 2011 Zoltan Bojthe
 *
 *              This program is free software; you can redistribute it
 *              and/or modify it under the terms of the GNU General Public
 *              License as published by the Free Software Foundation; either
 *              version 2 of the License, or (at your option) any later
 *              version.
 *              For further information see file COPYING
 *              in the top level directory
 ***************************************************************************
 * part of:     framework implementation developed by tkn
 **************************************************************************/

#include "ConnectionManager.h"
#include "MobilityBase.h"
#include "FWMath.h"

int NodeIndex=-1;
int NumOfNodes,SinkNodeAddr;
int PlaygroundSizeX,PlaygroundSizeY;

simsignal_t MobilityBase::mobilityStateChangedSignal = SIMSIGNAL_NULL;

static bool PositionSet=false;
struct NodesAttributes{
    double X_Position;
    double Y_Position;
    int Rank;
    int *Neighbors;
    int N_of_Ngh;
}*Sensor;
struct QUEUE
{
    int data;
    struct QUEUE* Link;
}*FRONT = NULL, *REAR = NULL;

void InitiatePositions(void)
{
   for (int i=0;i<NumOfNodes;i++)
    {
        Sensor[i].X_Position = intuniform(0,PlaygroundSizeX);
        Sensor[i].Y_Position = intuniform(0,PlaygroundSizeY);
        Sensor[i].N_of_Ngh = 0;
        Sensor[i].Rank = 32767;
    }
}

void CalculationOfNeighbours(void)
{
    int i,j,Location,x,y;
    double MaxDist = GetRadius();
    int *NeighborsTemp = new int[NumOfNodes];
    for (i=0;i<NumOfNodes;i++)
    {
        Location=0;
        for(j=0;j<NumOfNodes;j++)
           if(i!=j)
           {
               x=abs(Sensor[i].X_Position-Sensor[j].X_Position);
               y=abs(Sensor[i].Y_Position-Sensor[j].Y_Position);
               if((x*x+y*y)<=(MaxDist*MaxDist))
               {
                   Sensor[i].N_of_Ngh++;
                   NeighborsTemp[Location] = j;
                   Location++;
               }
           }
        for(j=0;j<Sensor[i].N_of_Ngh;j++)
        {
            Sensor[i].Neighbors[j]=NeighborsTemp[j];
            NeighborsTemp[j] = 0;
        }
    }
    delete [] NeighborsTemp;
}



void AddQ(int x)
{
    struct QUEUE* Temp = new struct QUEUE;
    Temp->data = x;
    Temp->Link = NULL;
    if(FRONT==NULL)
        FRONT = Temp;
    else
        REAR->Link = Temp;
    REAR = Temp;
}

int DeleteQ(void)
{
    if(FRONT==NULL)
    {
        EV<<"\n Queue is Empty, Press any key to continue...\n\n";
        exit(0);
    }
    struct QUEUE* Temp = FRONT;
    int x = FRONT->data;
    FRONT=FRONT->Link;
    delete Temp;
    return(x);
}

void OptRank(int sinkNodeAdd)
   {
       int i,j;
       Sensor[sinkNodeAdd].Rank = 1;
       AddQ(sinkNodeAdd);
       while(FRONT!=NULL)
      {
          i=DeleteQ();
          for(j=0;j<Sensor[i].N_of_Ngh;j++)
              if((i!=Sensor[i].Neighbors[j])&&(Sensor[Sensor[i].Neighbors[j]].Rank>Sensor[i].Rank+1))
              {
                  Sensor[Sensor[i].Neighbors[j]].Rank = Sensor[i].Rank+1;
                  AddQ(Sensor[i].Neighbors[j]);
              }
      }
   }

void NodesDeployment(int sinkNodeAdd)
{
    int MaxOptimalRank = 32767;
    int q=0;
    while(MaxOptimalRank == 32767)
    {
        MaxOptimalRank = 0;
        InitiatePositions();
        CalculationOfNeighbours();
        OptRank(sinkNodeAdd);
        MaxOptimalRank=Sensor[sinkNodeAdd].Rank;
        for(int i=0;i<NumOfNodes;i++)
             if(Sensor[i].Rank>MaxOptimalRank)  MaxOptimalRank=Sensor[i].Rank;
        q++;
    }
/*
    for(int i=0;i<NumOfNodes;i++)
    {
        EV<<"\n\nNode["<<i<<"]. Neighbors = ";
        for(int j=0;j<Sensor[i].N_of_Ngh;j++)
            EV<<Sensor[i].Neighbors[j]<<"  ";
    }
    EV<<"\n\n";
*/
//    EV<<"\n\nNumber of deployments =  "<<q<<"\n\n";

}


static bool parseIntTo(const char *s, double& destValue)
{
    if (!s || !*s)
        return false;

    char *endptr;
    int value = strtol(s, &endptr, 10);

    if (*endptr)
        return false;

    destValue = value;
    return true;
}

static bool isFiniteNumber(double value)
{
    return value <= DBL_MAX && value >= -DBL_MAX;
}

MobilityBase::MobilityBase()
{
    visualRepresentation = NULL;
    constraintAreaMin = Coord::ZERO;
    constraintAreaMax = Coord::ZERO;
    lastPosition = Coord::ZERO;
}

void MobilityBase::initialize(int stage)
{
//    EV<<"\n\n\n\n\t\t\tNode "<<getParentModule()->getIndex()<<"\n\n";
    BasicModule::initialize(stage);
    EV << "initializing MobilityBase stage " << stage << endl;
    if (stage == 0)
    {
        mobilityStateChangedSignal = registerSignal("mobilityStateChanged");
        UseRandomTopology = par("UseRandomTopology").boolValue();
        constraintAreaMin.x = par("constraintAreaMinX");
        constraintAreaMin.y = par("constraintAreaMinY");
        constraintAreaMin.z = par("constraintAreaMinZ");
        constraintAreaMax.x = par("constraintAreaMaxX");
        constraintAreaMax.y = par("constraintAreaMaxY");
        constraintAreaMax.z = par("constraintAreaMaxZ");
        visualRepresentation = findVisualRepresentation();
        if (visualRepresentation) {
            const char *s = visualRepresentation->getDisplayString().getTagArg("p", 2);
            if (s && *s)
                error("The coordinates of '%s' are invalid. Please remove automatic arrangement"
                      " (3rd argument of 'p' tag) from '@display' attribute.", visualRepresentation->getFullPath().c_str());
        }
        if((UseRandomTopology)&&(!PositionSet))
        {
            NumOfNodes = getParentModule()->getParentModule()->par( "numNodes" );
            SinkNodeAddr= getParentModule()->getParentModule()->par( "SinkNodeAddr" );
            Sensor = new NodesAttributes[NumOfNodes] ;
            for(int i=0;i<NumOfNodes;i++)
                Sensor[i].Neighbors = new int [NumOfNodes];
            PlaygroundSizeX = constraintAreaMax.x;
            PlaygroundSizeY = constraintAreaMax.y;
            NodesDeployment(SinkNodeAddr);
            PositionSet=true;
        }
        initializePosition();
    }
    else if (stage == 1)
    {
//        initializePosition();
        if (!isFiniteNumber(lastPosition.x) || !isFiniteNumber(lastPosition.y) || !isFiniteNumber(lastPosition.z))
            throw cRuntimeError("Mobility position is not a finite number after initialize (x=%g,y=%g,z=%g)", lastPosition.x, lastPosition.y, lastPosition.z);
        if (isOutside())
            throw cRuntimeError("Mobility position (x=%g,y=%g,z=%g) is outside the constraint area (%g,%g,%g - %g,%g,%g)",
                  lastPosition.x, lastPosition.y, lastPosition.z,
                  constraintAreaMin.x, constraintAreaMin.y, constraintAreaMin.z,
                  constraintAreaMax.x, constraintAreaMax.y, constraintAreaMax.z);
        EV << "initial position. x = " << lastPosition.x << " y = " << lastPosition.y << " z = " << lastPosition.z << endl;
        emitMobilityStateChangedSignal();
        updateVisualRepresentation();
    }
}

void MobilityBase::initializePosition()
{
    if(!UseRandomTopology)
    {
        // reading the coordinates from omnetpp.ini makes predefined scenarios a lot easier
        bool filled = false;
        if (hasPar("initialX") && hasPar("initialY") && hasPar("initialZ"))
        {
            lastPosition.x = x = par("initialX");
            lastPosition.y = y = par("initialY");
            lastPosition.z = z = par("initialZ");
            filled = true;
        }
        // not all mobility models have "initialX", "initialY" and "initialZ" parameters
        else
            if (hasPar("initFromDisplayString") && par("initFromDisplayString").boolValue() && visualRepresentation)
            {
               filled = parseIntTo(visualRepresentation->getDisplayString().getTagArg("p", 0), lastPosition.x)
                  && parseIntTo(visualRepresentation->getDisplayString().getTagArg("p", 1), lastPosition.y);
               if (filled)
                  lastPosition.z = 0;

            }
        if (!filled)
            lastPosition = getRandomPosition();
    }
    else lastPosition = getRandomPosition();

}

void MobilityBase::handleMessage(cMessage * message)
{
    if (message->isSelfMessage())
        handleSelfMessage(message);
    else
        throw cRuntimeError("Mobility modules can only receive self messages");
}

void MobilityBase::updateVisualRepresentation()
{
    if (ev.isGUI() && visualRepresentation)
    {
        EV << "visual position. x = " << lastPosition.x << " y = " << lastPosition.y << " z = " << lastPosition.z << endl;
        visualRepresentation->getDisplayString().setTagArg("p", 0, (long)lastPosition.x);
        visualRepresentation->getDisplayString().setTagArg("p", 1, (long)lastPosition.y);
    }
}

void MobilityBase::emitMobilityStateChangedSignal()
{
    emit(mobilityStateChangedSignal, this);
}

Coord MobilityBase::getRandomPosition()
{
    Coord p;
    if(UseRandomTopology)
    {
        NodeIndex++;
        p.x = x = Sensor[NodeIndex].X_Position;
        p.y = y = Sensor[NodeIndex].Y_Position;
        p.z = z = 0;
    }
    else
    {
        p.x = x = uniform(constraintAreaMin.x, constraintAreaMax.x);
        p.y = y = uniform(constraintAreaMin.y, constraintAreaMax.y);
        p.z = z = uniform(constraintAreaMin.z, constraintAreaMax.z);
    }
    return p;
}

bool MobilityBase::isOutside()
{
    return lastPosition.x < constraintAreaMin.x || lastPosition.x > constraintAreaMax.x
        || lastPosition.y < constraintAreaMin.y || lastPosition.y > constraintAreaMax.y
        || lastPosition.z < constraintAreaMin.z || lastPosition.z > constraintAreaMax.z;
}

static int reflect(double min, double max, double &coordinate, double &speed)
{
    double size = max - min;
    double value = coordinate - min;
    int sign = 1 - FWMath::modulo(floor(value / size), 2) * 2;
    ASSERT(sign == 1 || sign == -1);
    coordinate = FWMath::modulo(sign * value, size) + min;
    speed = sign * speed;
    return sign;
}

void MobilityBase::reflectIfOutside(Coord& targetPosition, Coord& speed, double& angle)
{
    int sign;
    double dummy;
    if (lastPosition.x < constraintAreaMin.x || constraintAreaMax.x < lastPosition.x) {
        sign = reflect(constraintAreaMin.x, constraintAreaMax.x, lastPosition.x, speed.x);
        reflect(constraintAreaMin.x, constraintAreaMax.x, targetPosition.x, dummy);
        angle = 90 + sign * (angle - 90);
    }
    if (lastPosition.y < constraintAreaMin.y || constraintAreaMax.y < lastPosition.y) {
        sign = reflect(constraintAreaMin.y, constraintAreaMax.y, lastPosition.y, speed.y);
        reflect(constraintAreaMin.y, constraintAreaMax.y, targetPosition.y, dummy);
        angle = sign * angle;
    }
    if (lastPosition.z < constraintAreaMin.z || constraintAreaMax.z < lastPosition.z) {
        sign = reflect(constraintAreaMin.z, constraintAreaMax.z, lastPosition.z, speed.z);
        reflect(constraintAreaMin.z, constraintAreaMax.z, targetPosition.z, dummy);
        // NOTE: angle is not affected
    }
}

static void wrap(double min, double max, double &coordinate)
{
    coordinate = FWMath::modulo(coordinate - min, max - min) + min;
}

void MobilityBase::wrapIfOutside(Coord& targetPosition)
{
    if (lastPosition.x < constraintAreaMin.x || constraintAreaMax.x < lastPosition.x) {
        wrap(constraintAreaMin.x, constraintAreaMax.x, lastPosition.x);
        wrap(constraintAreaMin.x, constraintAreaMax.x, targetPosition.x);
    }
    if (lastPosition.y < constraintAreaMin.y || constraintAreaMax.y < lastPosition.y) {
        wrap(constraintAreaMin.y, constraintAreaMax.y, lastPosition.y);
        wrap(constraintAreaMin.y, constraintAreaMax.y, targetPosition.y);
    }
    if (lastPosition.z < constraintAreaMin.z || constraintAreaMax.z < lastPosition.z) {
        wrap(constraintAreaMin.z, constraintAreaMax.z, lastPosition.z);
        wrap(constraintAreaMin.z, constraintAreaMax.z, targetPosition.z);
    }
}

void MobilityBase::placeRandomlyIfOutside(Coord& targetPosition)
{
    if (isOutside())
    {
        Coord newPosition = getRandomPosition();
        targetPosition += newPosition - lastPosition;
        lastPosition = newPosition;
    }
}

void MobilityBase::raiseErrorIfOutside()
{
    if (isOutside())
    {
        throw cRuntimeError("Mobility moved outside the area %g,%g,%g - %g,%g,%g (x=%g,y=%g,z=%g)",
              constraintAreaMin.x, constraintAreaMin.y, constraintAreaMin.z,
              constraintAreaMax.x, constraintAreaMax.y, constraintAreaMax.z,
              lastPosition.x, lastPosition.y, lastPosition.z);
    }
}

void MobilityBase::handleIfOutside(BorderPolicy policy, Coord& targetPosition, Coord& speed, double& angle)
{
    switch (policy)
    {
        case REFLECT:       reflectIfOutside(targetPosition, speed, angle); break;
        case WRAP:          wrapIfOutside(targetPosition); break;
        case PLACERANDOMLY: placeRandomlyIfOutside(targetPosition); break;
        case RAISEERROR:    raiseErrorIfOutside(); break;
        default:            throw cRuntimeError("Invalid outside policy=%d in module", policy, getFullPath().c_str());
    }
}
MobilityBase::~MobilityBase()
{
    if (getParentModule()->getIndex()==NumOfNodes-1)
    {
        for (int i=0;i<NumOfNodes;i++)
            delete [] Sensor[i].Neighbors;
        delete [] Sensor;
    }
}
