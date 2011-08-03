#include <cisstOpenNI/cisstOpenNISkeleton.h>
#include <cisstOpenNI/cisstOpenNI.h>
#include "cisstOpenNIData.h"

cisstOpenNISkeleton::cisstOpenNISkeleton(cisstOpenNI * openNI)
{

    points3D.resize(25);
    points2D.resize(25);
    confidence.resize(25);

    this->OpenNI = openNI;
    usrState = CNI_USR_IDLE;
    calState = CNI_USR_IDLE;


}

cisstOpenNISkeleton::~cisstOpenNISkeleton(){}

void cisstOpenNISkeleton::Update(int id)
{

    for(int i = 1; i < points3D.size();i++){
        XnSkeletonJointPosition pos;
        XnSkeletonJoint joint = XnSkeletonJoint(i);
        this->OpenNI->Data->usergenerator.GetSkeletonCap().GetSkeletonJointPosition(id,joint,pos);

        if (pos.fConfidence < 0.5 || pos.fConfidence < 0.5)
        {
            points3D[joint][0] = 0;
            points3D[joint][1] = 0;
            points3D[joint][2] = 0;

            points2D[joint][0] = 0;
            points2D[joint][1] = 0;

            confidence[joint] = false;

        }else{

            points3D[joint][0] = pos.position.X;
            points3D[joint][1] = pos.position.Y;
            points3D[joint][2] = pos.position.Z;

            XnPoint3D tempPt[1];
            tempPt[0] = pos.position;
            this->OpenNI->Data->depthgenerator.ConvertRealWorldToProjective(1, tempPt, tempPt);

            points2D[joint][0] = tempPt[0].X;
            points2D[joint][1] = tempPt[0].Y;
            confidence[joint] = true; 

        }
    }
    exists = true;

}

void cisstOpenNISkeleton::SetExists(bool val){
    exists = val;
}