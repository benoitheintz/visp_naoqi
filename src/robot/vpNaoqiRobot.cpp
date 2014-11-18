/****************************************************************************
 *
 * This file is part of the ViSP software.
 * Copyright (C) 2005 - 2014 by INRIA. All rights reserved.
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * ("GPL") version 2 as published by the Free Software Foundation.
 * See the file LICENSE.txt at the root directory of this source
 * distribution for additional information about the GNU GPL.
 *
 * For using ViSP with software that can not be combined with the GNU
 * GPL, please contact INRIA about acquiring a ViSP Professional
 * Edition License.
 *
 * See http://team.inria.fr/lagadic/visp for more information.
 *
 * This software was developed at:
 * INRIA Rennes - Bretagne Atlantique
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * France
 * http://team.inria.fr/lagadic
 *
 * If you have questions regarding the use of this file, please contact
 * INRIA at visp@inria.fr
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Description:
 * This class allows to the robot remotely.
 *
 * Authors:
 * Fabien Spindler
 * Giovanni Claudio
 *
 *****************************************************************************/


#include <visp/vpVelocityTwistMatrix.h>

#include <visp_naoqi/vpNaoqiRobot.h>

#include <alcommon/albroker.h>

#ifdef VISP_NAOQI_HAVE_MATAPOD
#  include <romeo.hh>
#  include <metapod/tools/print.hh>
#  include <metapod/tools/initconf.hh>
#  include <metapod/algos/jac_point_chain.hh>
#  include <metapod/tools/jcalc.hh>

using namespace metapod;

typedef double LocalFloatType;
typedef romeo<LocalFloatType> RomeoModel;
#endif

/*!
  Default constructor that set the default parameters as:
  - robot ip address: "198.18.0.1"
  - collision protection: enabled
  */
vpNaoqiRobot::vpNaoqiRobot()
  : m_motionProxy(NULL),m_proxy(NULL), m_robotIp("198.18.0.1"), m_isOpen(false), m_collisionProtection(true), m_robotName("")
{
}

/*!
  Destructor that call cleanup().
 */
vpNaoqiRobot::~vpNaoqiRobot()
{
  cleanup();
}

/*!
  Open the connection with the robot.
  All the parameters should be set before calling this function.
  \code
  #include <visp_naoqi/vpNaoqiRobot.h>

  int main()
  {
    vpNaoqiRobot robot;
    robot.setRobotIp("131.254.13.37");
    robot.open();
  }
  \endcode
 */
void vpNaoqiRobot::open()
{
  if (! m_isOpen) {
    cleanup();

    // Create a proxy to control the robot
    m_motionProxy = new AL::ALMotionProxy(m_robotIp);

    // Create a general proxy to motion to use new functions not implemented in the specialized proxy

    std::string 	myIP = "";		// IP du portable (voir /etc/hosts)
    int 	myPort = 0 ;			// Default broker port

    boost::shared_ptr<AL::ALBroker> broker = AL::ALBroker::createBroker("BrokerMotion", myIP, myPort, m_robotIp, 9559);
    m_proxy = new AL::ALProxy(broker, "ALMotion");

    int success;
    if (m_collisionProtection == false)
      success = m_motionProxy->setCollisionProtectionEnabled("Arms", "False");
    else
      m_motionProxy->setCollisionProtectionEnabled("Arms", "True");
    std::cout << "Collision protection is " << m_collisionProtection << std::endl;

    // Check the type of the robot
    AL::ALValue robotConfig = m_motionProxy->getRobotConfig();
    m_robotName = std::string(robotConfig[1][0]);

    if (m_robotName.find("romeo") != std::string::npos) {
      std::cout << "This robot is Romeo" << std::endl;
    }
    else if (m_robotName.find("nao") != std::string::npos) {
      std::cout << "This robot is Nao" << std::endl;
    }
    else if (m_robotName.find("pepper") != std::string::npos) {
      std::cout << "This robot is Pepper" << std::endl;
    }
    else {
      std::cout << "Unknown robot" << std::endl;
    }

    // Set Trapezoidal interpolation
    AL::ALValue config;

    AL::ALValue one = AL::ALValue::array(std::string("CONTROL_USE_ACCELERATION_INTERPOLATOR"),AL::ALValue(1));
    AL::ALValue two = AL::ALValue::array(std::string("CONTROL_JOINT_MAX_ACCELERATION"),AL::ALValue(5.0));

    config.arrayPush(one);
    config.arrayPush(two);

    m_motionProxy->setMotionConfig(config);

    //On nao, we have joint coupled limits (http://doc.aldebaran.com/2-1/family/robots/joints_robot.html) on the head and ankle.
    //Motion and DCM have clamping. We have to remove motion clamping.
    if (m_robotName.find("nao") != std::string::npos)
    {
      AL::ALValue config_;
      AL::ALValue setting = AL::ALValue::array(std::string("ENABLE_DCM_LIKE_CLAMPING"),AL::ALValue(0));
      config_.arrayPush(setting);
    }

    std::cout << "Trapezoidal interpolation is on " << std::endl;

    m_isOpen = true;
  }
}

void vpNaoqiRobot::cleanup()
{
  if (m_motionProxy != NULL) {
    delete m_motionProxy;
    m_motionProxy = NULL;
  }
  m_isOpen = false;
}

/*!
  Set the stiffness to a chain name, or to a specific joint.
  \param names :  Names the joints, chains, "Body", "JointActuators",
  "Joints" or "Actuators".
  \param stiffness : Stiffness parameter that should be between
  0 (no stiffness) and 1 (full stiffness).
 */
void vpNaoqiRobot::setStiffness(const AL::ALValue& names, float stiffness)
{
  m_motionProxy->setStiffnesses(names, AL::ALValue( stiffness ));
}

/*!
  Apply a velocity vector to a vector of joints.
  \param names :  Names the joints, chains, "Body", "JointActuators",
  "Joints" or "Actuators".
  \param jointVel : Joint velocity vector with values expressed in rad/s.
  \param verbose : If true activates printings.
 */

void vpNaoqiRobot::setVelocity(const AL::ALValue& names, const vpColVector &jointVel, bool verbose)
{
  std::vector<float> jointVel_(jointVel.size());
  for (unsigned int i=0; i< jointVel.size(); i++)
    jointVel_[i] = jointVel[i];
  setVelocity(names, jointVel_);
}

/*!
  Apply a velocity vector to a vector of joints.

  \todo Improve the function to be able to pass just one joint as names argument.

  \param names :  Names the joints, chains, "Body", "JointActuators",
  "Joints" or "Actuators".
  \param jointVel : Joint velocity vector with values expressed in rad/s.
  \param verbose : If true activates printings.
 */
void vpNaoqiRobot::setVelocity(const AL::ALValue &names, const AL::ALValue &jointVel, bool verbose)
{
  std::vector<std::string> jointNames;
  if (names.isString()) // Suppose to be a chain
    jointNames = m_motionProxy->getBodyNames(names);
  else if (names.isArray()) // Supposed to be a vector of joints
    jointNames = names; // it a vector of joints
  else
    throw vpRobotException (vpRobotException::readingParametersError,
                            "Unable to decode the joint chain.");

  if (jointNames.size() != jointVel.getSize() ) {
    throw vpRobotException (vpRobotException::readingParametersError,
                            "The dimensions of the joint array and the velocities array do not match.");
  }

  for (unsigned i = 0 ; i < jointVel.getSize() ; ++i)
  {
    std::string jointName = jointNames[i];
    if (verbose)
      std::cout << "Joint name: " << jointName << std::endl;

    float vel = jointVel[i];

    if (verbose)
      std::cout << "Desired velocity =  " << vel << "rad/s to the joint " << jointName << "." << std::endl;

    //Get the limits of the joint
    AL::ALValue limits = m_motionProxy->getLimits(jointName);

    AL::ALValue angle = AL::ALValue( 0.0f);
    unsigned int applymotion = 0;

    if (vel > 0.0f)
    {
      //Reach qMax
      angle = limits[0][1];
      applymotion = 1;
      if (verbose)
        std::cout << "Reach qMax (" << angle << ") ";
    }

    else if (vel < 0.0f)
    {
      //Reach qMin
      angle = limits[0][0];
      applymotion = 1;
      if (verbose)
        std::cout << "Reach qMin (" << angle << ") ";
    }

    if (applymotion)
    {
      float fraction = fabs( float (vel/float(limits[0][2])));
      if (fraction >= 1.0 )
      {
        if (verbose) {
          std::cout << "Given velocity is too high: " <<  vel << "rad/s for " << jointName << "." << std::endl;
          std::cout << "Max allowed is: " << limits[0][2] << "rad/s for "<< std::endl;
        }
        fraction = 1.0;
      }
      m_motionProxy->setAngles(jointName,angle,fraction);
      if (verbose)
        std::cout << "SET VELOCITY TO " <<  vel << "rad/s for " << jointName << "Angle: "<< angle
                  << ". Fraction " <<  fraction << std::endl;
    }
    else
      m_motionProxy->changeAngles(jointName,0.0f,0.1f);
  }
}

/*!
  Apply a velocity vector to a vector of joints. Use just one call to apply the velocities.
  NOT WORKING: In the C++ SDK is not implemented yet the function setAngles able to handle
  vector of fraction

  \todo Improve the function to be able to pass just one joint as names argument.

  \param names :  Names the joints, chains, "Body", "JointActuators",
  "Joints" or "Actuators".
  \param jointVel : Joint velocity vector with values expressed in rad/s.
  \param verbose : If true activates printings.
 */
void vpNaoqiRobot::setVelocity_one_call(const AL::ALValue &names, const AL::ALValue &jointVel, bool verbose)
{
  std::vector<std::string> jointNames;
  if (names.isString()) // Suppose to be a chain
    jointNames = m_motionProxy->getBodyNames(names);
  else if (names.isArray()) // Supposed to be a vector of joints
    jointNames = names; // it a vector of joints
  else
    throw vpRobotException (vpRobotException::readingParametersError,
                            "Unable to decode the joint chain.");

  if (jointNames.size() != jointVel.getSize() ) {
    throw vpRobotException (vpRobotException::readingParametersError,
                            "The dimensions of the joint array and the velocities array do not match.");
  }

  AL::ALValue jointListStop;
  AL::ALValue jointListMove;
  AL::ALValue angles;
  std::vector<float> fractions;

  for (unsigned i = 0 ; i < jointVel.getSize() ; ++i)
  {
    std::string jointName = jointNames[i];
    if (verbose)
      std::cout << "Joint name: " << jointName << std::endl;

    float vel = jointVel[i];

    if (verbose)
      std::cout << "Desired velocity =  " << vel << "rad/s to the joint " << jointName << "." << std::endl;

    //Get the limits of the joint
    AL::ALValue limits = m_motionProxy->getLimits(jointName);


    if (vel == 0.0f)
    {
      if (verbose)
        std::cout << "Stop the joint" << std::endl ;

      jointListStop.arrayPush(jointName);
    }
    else
    {

      if (vel > 0.0f)
      {
        //Reach qMax
        angles.arrayPush(limits[0][1]);
        if (verbose)
          std::cout << "Reach qMax (" << limits[0][1] << ") ";
      }

      else if (vel < 0.0f)
      {
        //Reach qMin
        angles.arrayPush(limits[0][0]);
        if (verbose)
          std::cout << "Reach qMin (" << limits[0][0] << ") ";
      }


      jointListMove.arrayPush(jointName);
      float fraction = fabs( float (vel/float(limits[0][2])));
      if (fraction >= 1.0 )
      {
        if (verbose) {
          std::cout << "Given velocity is too high: " <<  vel << "rad/s for " << jointName << "." << std::endl;
          std::cout << "Max allowed is: " << limits[0][2] << "rad/s for "<< std::endl;
        }
        fraction = 1.0;
      }

      fractions.push_back(fraction);

    }
  }
  std::cout << "Apply Velocity to joints " << jointListMove << std::endl;
  std::cout << "Stop List joints: " << jointListStop << std::endl;
  std::cout << "with fractions " << angles << std::endl;
  std::cout << "to angles " << fractions << std::endl;

  if (jointListMove.getSize()>0)
  {
    m_proxy->callVoid("setAngles", jointListMove, angles, fractions);
    //m_motionProxy->setAngles(jointListMove,angles,fractions);
  }

  if (jointListStop.getSize()>0)
  {
    AL::ALValue zeros =AL::ALValue::array(0.0f);
    zeros.arraySetSize(jointListStop.getSize());

    m_proxy->callVoid("changeAngles", jointListStop, zeros, 0.1f);
    // m_motionProxy->changeAngles(jointListStop,zeros,0.1f);
  }
}

/*!
  Stop the velocity applied to the joints.
  \param names :  Names the joints, chains, "Body", "JointActuators",
  "Joints" or "Actuators" to stop.
 */
void vpNaoqiRobot::stop(const AL::ALValue &names)
{
  std::vector<std::string> jointNames;
  if (names.isString()) // Suppose to be a chain
    jointNames = m_motionProxy->getBodyNames(names);
  else if (names.isArray()) // Supposed to be a vector of joints
    jointNames = names; // it a vector of joints
  else
    throw vpRobotException (vpRobotException::readingParametersError,
                            "Unable to decode the joint chain.");

  std::vector<float> angles;
  for (unsigned i = 0 ; i < jointNames.size() ; ++i)
    m_motionProxy->changeAngles(jointNames[i], 0.0f, 0.1f);
  //      {
  //    angles = m_motionProxy->getAngles(jointNames[i],true);
  //    m_motionProxy->setAngles(jointNames[i],angles[0],1.0);

  //  }

}

/*!
  Get the name of all the joints of the chain.

  \param names : Names the joints, chains, "Body", "JointActuators",
  "Joints" or "Actuators".

  \return The name of the joints.
 */
std::vector<std::string>
vpNaoqiRobot::getBodyNames(const std::string &names) const
{
  if (! m_isOpen)
    throw vpRobotException (vpRobotException::readingParametersError,
                            "The connexion with the robot was not open");
  std::vector<std::string> jointNames = m_motionProxy->getBodyNames(names);
  return jointNames;
}

/*!
  Get minimal joint values for a joint chain.

  \return A vector that contains the minimal joint values
  of the chain. All the values are expressed in radians.

  \param names : Names the joints, chains, "Body", "JointActuators",
  "Joints" or "Actuators".

  \return The min angle of each joint of the chain.
*/
vpColVector
vpNaoqiRobot::getJointMin(const AL::ALValue& names) const
{
  if (! m_isOpen)
    throw vpRobotException (vpRobotException::readingParametersError,
                            "The connexion with the robot was not open");
  AL::ALValue limits = m_motionProxy->getLimits(names);
  vpColVector min_limits(limits.getSize());
  for (unsigned int i=0; i<limits.getSize(); i++)
  {
    min_limits[i] = limits[i][0];
  }
  return min_limits;
}

/*!
  Get minimal joint values for a joint chain.

  \return A vector that contains the minimal joint values
  of the chain. All the values are expressed in radians.

  \param names : Names the joints, chains, "Body", "JointActuators",
  "Joints" or "Actuators".

  \return The min angle of each joint of the chain.
*/
vpColVector
vpNaoqiRobot::getJointMax(const AL::ALValue& names) const
{
  if (! m_isOpen)
    throw vpRobotException (vpRobotException::readingParametersError,
                            "The connexion with the robot was not open");
  AL::ALValue limits = m_motionProxy->getLimits(names);
  vpColVector max_limits(limits.getSize());
  for (unsigned int i=0; i<limits.getSize(); i++)
  {
    max_limits[i] = limits[i][1];
  }
  return max_limits;
}


/*!
Gets the angles of the joints

 \return Joint angles in radians.

 \param names : Names the joints, chains, “Body”, “JointActuators”, “Joints” or “Actuators”.
        useSensors – If true, sensor angles will be returned

*/

vpColVector vpNaoqiRobot::getPosition(const AL::ALValue& names, const bool& useSensors) const
{
  std::vector<float> sensorAngles = m_motionProxy->getAngles(names, useSensors);
  vpColVector q(sensorAngles.size());
  for(unsigned int i=0; i<sensorAngles.size(); i++)
    q[i] = sensorAngles[i];
  return q;
}


/*!
Set the position of the joints

 \param names:  The name or names of joints, chains, “Body”, “JointActuators”, “Joints” or “Actuators”.

 \param angles: One or more angles in radians

 \param fractionMaxSpeed – The fraction of maximum speed to use

*/

void vpNaoqiRobot::setPosition(const AL::ALValue& names, const AL::ALValue& angles, const float& fractionMaxSpeed)
{ 
  m_motionProxy->setAngles(names, angles, fractionMaxSpeed);
}

/*!
Set the position of the joints using vpColVector of Visp

 \param names:  The name or names of joints, chains, “Body”, “JointActuators”, “Joints” or “Actuators”.

 \param jointPosition: One or more angles in radians (vpColVector)

 \param fractionMaxSpeed – The fraction of maximum speed to use

*/
void vpNaoqiRobot::setPosition(const AL::ALValue& names, const vpColVector &jointPosition, const float& fractionMaxSpeed)
{
  std::vector<float> angles(jointPosition.size());
  for (unsigned int i=0; i<angles.size(); i++)
    angles[i] = jointPosition[i];

  m_motionProxy->setAngles(names, angles, fractionMaxSpeed);
}

/*!
  Get Jacobian eJe for a joint chain.

  \return Jacobian eJe starting from the torso

  \param chainName : Name of the ChainName, can be "Head", "LArm" or "RArm".

*/
vpMatrix vpNaoqiRobot::get_eJe(const std::string &chainName) const
{
  vpMatrix eJe;

  if (chainName == "Head")
  {
#ifdef VISP_NAOQI_HAVE_MATAPOD
    //Jacobian matrix w.r.t the torso
    vpMatrix tJe;

    //confVector q for Romeo has size 24 (6 + 18dof of the robot, we don't consider the Legs and the fingers)
    RomeoModel::confVector q;

    // Get the names of the joints in the chain we want to control
    std::vector<std::string> jointNames = m_motionProxy->getBodyNames(chainName);

    // Get the angles of the joints in the chain we want to control
    std::vector<float> qmp = m_motionProxy->getAngles("Head",true);

    const unsigned int nJoints= qmp.size();

    //Resize the Jacobians
    eJe.resize(6,nJoints);
    tJe.resize(6,nJoints);

    // Create a robot instance of Metapod
    RomeoModel robot;

    //Get the index of the position of the q of the first joint of the chain in the confVector
    int index_confVec = (boost::fusion::at_c<RomeoModel::NeckYawLink>(robot.nodes).q_idx);


    // Copy the angle values of the joint in the confVector in the right position
    // In this case is +6 because in the first 6 positions there is the FreeFlyer
    for(unsigned int i=0;i<nJoints;i++)
      q[i+index_confVec] = qmp[i];

    // Compute the Jacobian tJe
    jcalc< RomeoModel>::run(robot, q, RomeoModel::confVector::Zero());

    static const bool includeFreeFlyer = true;
    static const int offset = 0;
    typedef jac_point_chain<RomeoModel, RomeoModel::torso, RomeoModel::HeadRollLink, offset, includeFreeFlyer> jac;
    jac::Jacobian J = jac::Jacobian::Zero();
    jac::run(robot, q, Vector3dTpl<LocalFloatType>::Type(0,0,0), J);

    for(unsigned int i=0;i<3;i++)
      for(unsigned int j=0;j<nJoints;j++)
      {
        tJe[i][j]=J(i+3, index_confVec +j);
        tJe[i+3][j]=J(i, index_confVec +j);

      }

    //std::cout << "metapod_Jac:" <<std::endl << J;

    // Now we want to transform tJe to eJe


    vpHomogeneousMatrix torsoMHeadRoll(m_motionProxy->getTransform(jointNames[nJoints-1], 0, true));// get transformation  matrix between torso and HeadRoll


    vpVelocityTwistMatrix HeadRollVLtorso(torsoMHeadRoll.inverse());

    for(unsigned int i=0; i< 3; i++)
      for(unsigned int j=0; j< 3; j++)
        HeadRollVLtorso[i][j+3] = 0;


    // Transform the matrix
    eJe = HeadRollVLtorso *tJe;

#else
    throw vpRobotException (vpRobotException::readingParametersError,
                            "Metapod is not installed");
#endif


  }

  //  else if (chainName == "Head")
  //  {
  //    std::vector<float> q = m_motionProxy->getAngles(chainName,true);

  //    //std::cout << "Joint value:" << q << std::endl;

  //    const unsigned int nJoints= q.size();

  //    eJe.resize(6,nJoints);

  //    double d3 = 0.09511;

  //    eJe[0][0]= d3*cos(q[4])*sin(q[2]);
  //    eJe[1][0]= -d3*sin(q[2])*sin(q[4]);
  //    eJe[2][0]= 0;
  //    eJe[3][0]= cos(q[2] + q[3])*sin(q[4]);
  //    eJe[4][0]= cos(q[2] + q[3])*cos(q[4]);
  //    eJe[5][0]=  -sin(q[2] + q[3]);

  //    eJe[0][1]= d3*sin(q[3])*sin(q[4]);
  //    eJe[1][1]= d3*cos(q[4])*sin(q[3]);
  //    eJe[2][1]= d3*cos(q[3]);
  //    eJe[3][1]=  cos(q[4]);
  //    eJe[4][1]= -sin(q[4]);
  //    eJe[5][1]= 0;

  //    eJe[0][2]= 0;
  //    eJe[1][2]= 0;
  //    eJe[2][2]= 0;
  //    eJe[3][2]= cos(q[4]);
  //    eJe[4][2]= -sin(q[4]);
  //    eJe[5][2]= 0;

  //    eJe[0][3]= 0;
  //    eJe[1][3]= 0;
  //    eJe[2][3]= 0;
  //    eJe[3][3]= 0;
  //    eJe[4][3]= 0;
  //    eJe[5][3]= 1;
  //  }

  else if (chainName == "LArm")
  {
#ifdef VISP_NAOQI_HAVE_MATAPOD
    //Jacobian matrix w.r.t the torso
    vpMatrix tJe;

    //confVector q for Romeo has size 24 (6 + 18dof of the robot, we don't consider the Legs and the fingers)
    RomeoModel::confVector q;

    // Get the names of the joints in the chain we want to control
    std::vector<std::string> jointNames = m_motionProxy->getBodyNames(chainName);
    jointNames.pop_back(); // Delete last joints LHand, that we don't consider in the servo

    // Get the angles of the joints in the chain we want to control
    std::vector<float> qmp = m_motionProxy->getAngles(chainName,true);
    qmp.pop_back(); // we don't consider the last joint LHand

    const unsigned int nJoints = qmp.size();

    //Resize the Jacobians
    eJe.resize(6,nJoints);
    tJe.resize(6,nJoints);

    // Create a robot instance of Metapod
    RomeoModel robot;

    //Get the index of the position of the q of the first joint of the chain in the confVector
    int index_confVec = (boost::fusion::at_c<RomeoModel::LShoulderPitchLink>(robot.nodes).q_idx);

    // Copy the angle values of the joint in the confVector in the right position
    // In this case is +6 because in the first 6 positions there is the FreeFlyer
    for(unsigned int i=0;i<nJoints;i++)
      q[i+index_confVec] = qmp[i];

    // Compute the Jacobian tJe
    jcalc< RomeoModel>::run(robot, q, RomeoModel::confVector::Zero());

    static const bool includeFreeFlyer = false;
    static const int offset = 0;
    typedef jac_point_chain<RomeoModel, RomeoModel::torso, RomeoModel::l_wrist, offset, includeFreeFlyer> jac;
    jac::Jacobian J = jac::Jacobian::Zero();
    jac::run(robot, q, Vector3dTpl<LocalFloatType>::Type(0,0,0), J);

    for(unsigned int i=0;i<3;i++)
      for(unsigned int j=0;j<nJoints;j++)
      {
        tJe[i][j]=J(i+3,RomeoModel::torso+j);
        tJe[i+3][j]=J(i,RomeoModel::torso+j);

      }

    //std::cout << "metapod_Jac:" <<std::endl << J << std::endl;

    // Now we want to transform tJe to eJe

    vpHomogeneousMatrix torsoMLWristP(m_motionProxy->getTransform(jointNames[nJoints-1], 0, true));

    vpVelocityTwistMatrix torsoVLWristP(torsoMLWristP.inverse());

    for(unsigned int i=0; i< 3; i++)
      for(unsigned int j=0; j< 3; j++)
        torsoVLWristP[i][j+3] = 0;


    // Transform the matrix
    eJe = torsoVLWristP *tJe;
#else
    throw vpRobotException (vpRobotException::readingParametersError,
                            "Metapod is not installed");

#endif // #ifdef VISP_NAOQI_HAVE_MATAPOD
  }


  else if (chainName == "RArm")
  {
#ifdef VISP_NAOQI_HAVE_MATAPOD
    //Jacobian matrix w.r.t the torso
    vpMatrix tJe;

    //confVector q for Romeo has size 24 (6 + 18dof of the robot, we don't consider the Legs and the fingers)
    RomeoModel::confVector q;

    // Get the names of the joints in the chain we want to control
    std::vector<std::string> jointNames = m_motionProxy->getBodyNames(chainName);
    jointNames.pop_back(); // Delete last joints LHand, that we don't consider in the servo

    // Get the angles of the joints in the chain we want to control
    std::vector<float> qmp = m_motionProxy->getAngles(chainName,true);
    qmp.pop_back(); // we don't consider the last joint LHand

    const unsigned int nJoints = qmp.size();

    //Resize the Jacobians
    eJe.resize(6,nJoints);
    tJe.resize(6,nJoints);

    // Create a robot instance of Metapod
    RomeoModel robot;

    //Get the index of the position of the q of the first joint of the chain in the confVector
    int index_confVec = (boost::fusion::at_c<RomeoModel::RShoulderPitchLink>(robot.nodes).q_idx);

    // Copy the angle values of the joint in the confVector in the right position
    // In this case is +6 because in the first 6 positions there is the FreeFlyer
    for(unsigned int i=0;i<nJoints;i++)
      q[i+index_confVec] = qmp[i];

    // Compute the Jacobian tJe
    jcalc< RomeoModel>::run(robot, q, RomeoModel::confVector::Zero());

    static const bool includeFreeFlyer = true;
    static const int offset = 0;
    typedef jac_point_chain<RomeoModel, RomeoModel::torso, RomeoModel::r_wrist, offset, includeFreeFlyer> jac;
    jac::Jacobian J = jac::Jacobian::Zero();
    jac::run(robot, q, Vector3dTpl<LocalFloatType>::Type(0,0,0), J);

    for(unsigned int i=0;i<3;i++)
      for(unsigned int j=0;j<nJoints;j++)
      {
        tJe[i][j]=J(i+3,index_confVec+j);
        tJe[i+3][j]=J(i,index_confVec+j);

      }


    // Now we want to transform tJe to eJe

    vpHomogeneousMatrix torsoMRWristP(m_motionProxy->getTransform(jointNames[nJoints-1], 0, true));


    vpVelocityTwistMatrix torsoVRWristP(torsoMRWristP.inverse());

    for(unsigned int i=0; i< 3; i++)
      for(unsigned int j=0; j< 3; j++)
        torsoVRWristP[i][j+3] = 0;


    // Transform the matrix
    eJe = torsoVRWristP *tJe;
#else
    throw vpRobotException (vpRobotException::readingParametersError,
                            "Metapod is not installed");

#endif // #ifdef VISP_NAOQI_HAVE_MATAPOD
  }

  else if (chainName == "LArm_old")
  {

    std::vector<float> q = m_motionProxy->getAngles(chainName,true);

    q.pop_back(); // we don't consider the last joint LHand

    //std::cout << "Joint value:" << q << std::endl;

    const unsigned int nJoints = q.size();

    float q10, q11, q12, q13, q14, q15;

    q10 = q[1]; //LShoulderYaw
    q11 = q[2]; //LElbowRoll
    q12 = q[3]; //LElbowYaw
    q13 = q[4]; //LWristRoll
    q14 = q[5]; //LWristYaw
    q15 = q[6]; //LWristPitch


    float g9 = 2.7053 ;
    float b9 = 0.0279;
    float alpha9 = 1.729254202933720;
    float d9 =  0.0758;
    float phi9 = 3.068152048225621;
    float phi10 = 0.430457;
    float phi11 = 0.17452;
    float r9 = 0.1765;
    float r11 =  0.205;
    float r13 =  0.1823;

    eJe.resize(6,nJoints);

    eJe[0][0]=r13*sin(phi11 + q11)*sin(phi10)*sin(q10)*sin(q13)*sin(q15) - r13*cos(phi10)*cos(q13)*sin(q10)*sin(q12)*sin(q15) - r13*cos(q10)*cos(q13)*sin(phi10)*sin(q12)*sin(q15) + r11*cos(phi11 + q11)*cos(phi10)*cos(q10)*cos(q13)*sin(q15) - r11*cos(phi11 + q11)*cos(q13)*sin(phi10)*sin(q10)*sin(q15) - r13*sin(phi11 + q11)*cos(phi10)*cos(q10)*sin(q13)*sin(q15) + r13*cos(phi10)*cos(q15)*sin(q10)*sin(q12)*sin(q13)*sin(q14) + r13*cos(q10)*cos(q15)*sin(phi10)*sin(q12)*sin(q13)*sin(q14) + r13*cos(phi11 + q11)*cos(phi10)*cos(q10)*cos(q12)*cos(q13)*sin(q15) - r11*sin(phi11 + q11)*cos(phi10)*cos(q10)*cos(q14)*cos(q15)*sin(q12) - r11*cos(phi11 + q11)*cos(phi10)*cos(q10)*cos(q15)*sin(q13)*sin(q14) - r13*sin(phi11 + q11)*cos(phi10)*cos(q10)*cos(q13)*cos(q15)*sin(q14) - r11*sin(phi11 + q11)*cos(phi10)*cos(q10)*cos(q12)*sin(q13)*sin(q15) - r13*cos(phi11 + q11)*cos(q12)*cos(q13)*sin(phi10)*sin(q10)*sin(q15) + r11*sin(phi11 + q11)*cos(q14)*cos(q15)*sin(phi10)*sin(q10)*sin(q12) + r11*cos(phi11 + q11)*cos(q15)*sin(phi10)*sin(q10)*sin(q13)*sin(q14) + r13*sin(phi11 + q11)*cos(q13)*cos(q15)*sin(phi10)*sin(q10)*sin(q14) + r11*sin(phi11 + q11)*cos(q12)*sin(phi10)*sin(q10)*sin(q13)*sin(q15) - r11*sin(phi11 + q11)*cos(phi10)*cos(q10)*cos(q12)*cos(q13)*cos(q15)*sin(q14) - r13*cos(phi11 + q11)*cos(phi10)*cos(q10)*cos(q12)*cos(q15)*sin(q13)*sin(q14) + r11*sin(phi11 + q11)*cos(q12)*cos(q13)*cos(q15)*sin(phi10)*sin(q10)*sin(q14) + r13*cos(phi11 + q11)*cos(q12)*cos(q15)*sin(phi10)*sin(q10)*sin(q13)*sin(q14);
    eJe[0][1]=r11*sin(phi11 + q11)*cos(q13)*sin(q15) + r13*cos(phi11 + q11)*sin(q13)*sin(q15) + r11*cos(phi11 + q11)*cos(q14)*cos(q15)*sin(q12) + r13*cos(phi11 + q11)*cos(q13)*cos(q15)*sin(q14) + r11*cos(phi11 + q11)*cos(q12)*sin(q13)*sin(q15) + r13*sin(phi11 + q11)*cos(q12)*cos(q13)*sin(q15) - r11*sin(phi11 + q11)*cos(q15)*sin(q13)*sin(q14) + r11*cos(phi11 + q11)*cos(q12)*cos(q13)*cos(q15)*sin(q14) - r13*sin(phi11 + q11)*cos(q12)*cos(q15)*sin(q13)*sin(q14);
    eJe[0][2]=-r13*sin(q12)*(cos(q13)*sin(q15) - cos(q15)*sin(q13)*sin(q14));
    eJe[0][3]=r13*sin(q13)*sin(q15) + r13*cos(q13)*cos(q15)*sin(q14);
    eJe[0][4]=0;
    eJe[0][5]=0;
    eJe[0][6]=0;
    eJe[1][0]=r13*sin(phi11 + q11)*cos(q15)*sin(phi10)*sin(q10)*sin(q13) - r13*cos(phi10)*cos(q13)*cos(q15)*sin(q10)*sin(q12) - r13*cos(q10)*cos(q13)*cos(q15)*sin(phi10)*sin(q12) + r11*cos(phi11 + q11)*cos(phi10)*cos(q10)*cos(q13)*cos(q15) - r11*cos(phi11 + q11)*cos(q13)*cos(q15)*sin(phi10)*sin(q10) - r13*sin(phi11 + q11)*cos(phi10)*cos(q10)*cos(q15)*sin(q13) - r13*cos(phi10)*sin(q10)*sin(q12)*sin(q13)*sin(q14)*sin(q15) - r13*cos(q10)*sin(phi10)*sin(q12)*sin(q13)*sin(q14)*sin(q15) + r13*cos(phi11 + q11)*cos(phi10)*cos(q10)*cos(q12)*cos(q13)*cos(q15) - r11*sin(phi11 + q11)*cos(phi10)*cos(q10)*cos(q12)*cos(q15)*sin(q13) - r13*cos(phi11 + q11)*cos(q12)*cos(q13)*cos(q15)*sin(phi10)*sin(q10) + r11*sin(phi11 + q11)*cos(phi10)*cos(q10)*cos(q14)*sin(q12)*sin(q15) + r11*cos(phi11 + q11)*cos(phi10)*cos(q10)*sin(q13)*sin(q14)*sin(q15) + r13*sin(phi11 + q11)*cos(phi10)*cos(q10)*cos(q13)*sin(q14)*sin(q15) + r11*sin(phi11 + q11)*cos(q12)*cos(q15)*sin(phi10)*sin(q10)*sin(q13) - r11*sin(phi11 + q11)*cos(q14)*sin(phi10)*sin(q10)*sin(q12)*sin(q15) - r11*cos(phi11 + q11)*sin(phi10)*sin(q10)*sin(q13)*sin(q14)*sin(q15) - r13*sin(phi11 + q11)*cos(q13)*sin(phi10)*sin(q10)*sin(q14)*sin(q15) + r11*sin(phi11 + q11)*cos(phi10)*cos(q10)*cos(q12)*cos(q13)*sin(q14)*sin(q15) + r13*cos(phi11 + q11)*cos(phi10)*cos(q10)*cos(q12)*sin(q13)*sin(q14)*sin(q15) - r11*sin(phi11 + q11)*cos(q12)*cos(q13)*sin(phi10)*sin(q10)*sin(q14)*sin(q15) - r13*cos(phi11 + q11)*cos(q12)*sin(phi10)*sin(q10)*sin(q13)*sin(q14)*sin(q15);
    eJe[1][1]=r11*sin(phi11 + q11)*cos(q13)*cos(q15) + r13*cos(phi11 + q11)*cos(q15)*sin(q13) + r11*cos(phi11 + q11)*cos(q12)*cos(q15)*sin(q13) + r13*sin(phi11 + q11)*cos(q12)*cos(q13)*cos(q15) - r11*cos(phi11 + q11)*cos(q14)*sin(q12)*sin(q15) - r13*cos(phi11 + q11)*cos(q13)*sin(q14)*sin(q15) + r11*sin(phi11 + q11)*sin(q13)*sin(q14)*sin(q15) + r13*sin(phi11 + q11)*cos(q12)*sin(q13)*sin(q14)*sin(q15) - r11*cos(phi11 + q11)*cos(q12)*cos(q13)*sin(q14)*sin(q15);
    eJe[1][2]=-r13*sin(q12)*(cos(q13)*cos(q15) + sin(q13)*sin(q14)*sin(q15));
    eJe[1][3]=r13*cos(q15)*sin(q13) - r13*cos(q13)*sin(q14)*sin(q15);
    eJe[1][4]=0;
    eJe[1][5]=0;
    eJe[1][6]=0;
    eJe[2][0]=r13*cos(phi10)*cos(q14)*sin(q10)*sin(q12)*sin(q13) - r11*sin(phi11 + q11)*sin(phi10)*sin(q10)*sin(q12)*sin(q14) + r13*cos(q10)*cos(q14)*sin(phi10)*sin(q12)*sin(q13) - r11*cos(phi11 + q11)*cos(phi10)*cos(q10)*cos(q14)*sin(q13) - r13*sin(phi11 + q11)*cos(phi10)*cos(q10)*cos(q13)*cos(q14) + r11*sin(phi11 + q11)*cos(phi10)*cos(q10)*sin(q12)*sin(q14) + r11*cos(phi11 + q11)*cos(q14)*sin(phi10)*sin(q10)*sin(q13) + r13*sin(phi11 + q11)*cos(q13)*cos(q14)*sin(phi10)*sin(q10) - r11*sin(phi11 + q11)*cos(phi10)*cos(q10)*cos(q12)*cos(q13)*cos(q14) - r13*cos(phi11 + q11)*cos(phi10)*cos(q10)*cos(q12)*cos(q14)*sin(q13) + r11*sin(phi11 + q11)*cos(q12)*cos(q13)*cos(q14)*sin(phi10)*sin(q10) + r13*cos(phi11 + q11)*cos(q12)*cos(q14)*sin(phi10)*sin(q10)*sin(q13);
    eJe[2][1]=r13*cos(phi11 + q11)*cos(q13)*cos(q14) - r11*cos(phi11 + q11)*sin(q12)*sin(q14) - r11*sin(phi11 + q11)*cos(q14)*sin(q13) - r13*sin(phi11 + q11)*cos(q12)*cos(q14)*sin(q13) + r11*cos(phi11 + q11)*cos(q12)*cos(q13)*cos(q14);
    eJe[2][2]=r13*cos(q14)*sin(q12)*sin(q13);
    eJe[2][3]=r13*cos(q13)*cos(q14);
    eJe[2][4]=0;
    eJe[2][5]=0;
    eJe[2][6]=0;
    eJe[3][0]=cos(phi10 + q10)*sin(phi11 + q11)*cos(q13)*sin(q15) + sin(phi10 + q10)*cos(q12)*cos(q14)*cos(q15) - sin(phi10 + q10)*sin(q12)*sin(q13)*sin(q15) + cos(phi10 + q10)*cos(phi11 + q11)*cos(q14)*cos(q15)*sin(q12) + cos(phi10 + q10)*cos(phi11 + q11)*cos(q12)*sin(q13)*sin(q15) - cos(phi10 + q10)*sin(phi11 + q11)*cos(q15)*sin(q13)*sin(q14) - sin(phi10 + q10)*cos(q13)*cos(q15)*sin(q12)*sin(q14) + cos(phi10 + q10)*cos(phi11 + q11)*cos(q12)*cos(q13)*cos(q15)*sin(q14);
    eJe[3][1]=sin(phi11 + q11)*cos(q14)*cos(q15)*sin(q12) - cos(phi11 + q11)*cos(q13)*sin(q15) + cos(phi11 + q11)*cos(q15)*sin(q13)*sin(q14) + sin(phi11 + q11)*cos(q12)*sin(q13)*sin(q15) + sin(phi11 + q11)*cos(q12)*cos(q13)*cos(q15)*sin(q14);
    eJe[3][2]=cos(q12)*cos(q14)*cos(q15) - sin(q12)*sin(q13)*sin(q15) - cos(q13)*cos(q15)*sin(q12)*sin(q14);
    eJe[3][3]=cos(q15)*sin(q13)*sin(q14) - cos(q13)*sin(q15);
    eJe[3][4]=cos(q14)*cos(q15);
    eJe[3][5]=-sin(q15);
    eJe[3][6]=0;
    eJe[4][0]=cos(phi10 + q10)*sin(phi11 + q11)*cos(q13)*cos(q15) - sin(phi10 + q10)*cos(q12)*cos(q14)*sin(q15) - sin(phi10 + q10)*cos(q15)*sin(q12)*sin(q13) + cos(phi10 + q10)*cos(phi11 + q11)*cos(q12)*cos(q15)*sin(q13) - cos(phi10 + q10)*cos(phi11 + q11)*cos(q14)*sin(q12)*sin(q15) + cos(phi10 + q10)*sin(phi11 + q11)*sin(q13)*sin(q14)*sin(q15) + sin(phi10 + q10)*cos(q13)*sin(q12)*sin(q14)*sin(q15) - cos(phi10 + q10)*cos(phi11 + q11)*cos(q12)*cos(q13)*sin(q14)*sin(q15);
    eJe[4][1]=sin(phi11 + q11)*cos(q12)*cos(q15)*sin(q13) - cos(phi11 + q11)*cos(q13)*cos(q15) - sin(phi11 + q11)*cos(q14)*sin(q12)*sin(q15) - cos(phi11 + q11)*sin(q13)*sin(q14)*sin(q15) - sin(phi11 + q11)*cos(q12)*cos(q13)*sin(q14)*sin(q15);
    eJe[4][2]=cos(q13)*sin(q12)*sin(q14)*sin(q15) - cos(q15)*sin(q12)*sin(q13) - cos(q12)*cos(q14)*sin(q15);
    eJe[4][3]=- cos(q13)*cos(q15) - sin(q13)*sin(q14)*sin(q15);
    eJe[4][4]=-cos(q14)*sin(q15);
    eJe[4][5]=-cos(q15);
    eJe[4][6]=0;
    eJe[5][0]=cos(phi10 + q10)*cos(phi11 + q11)*cos(q12)*cos(q13)*cos(q14) - cos(phi10 + q10)*cos(phi11 + q11)*sin(q12)*sin(q14) - cos(phi10 + q10)*sin(phi11 + q11)*cos(q14)*sin(q13) - sin(phi10 + q10)*cos(q13)*cos(q14)*sin(q12) - sin(phi10 + q10)*cos(q12)*sin(q14);
    eJe[5][1]=cos(phi11 + q11)*cos(q14)*sin(q13) - sin(phi11 + q11)*sin(q12)*sin(q14) + sin(phi11 + q11)*cos(q12)*cos(q13)*cos(q14);
    eJe[5][2]=- cos(q12)*sin(q14) - cos(q13)*cos(q14)*sin(q12);
    eJe[5][3]=cos(q14)*sin(q13);
    eJe[5][4]=-sin(q14);
    eJe[5][5]=0;
    eJe[5][6]=1;
  }
  else
  {
    throw vpRobotException (vpRobotException::readingParametersError,
                            "End-effector name not recognized. Please choose one above 'Head', 'LArm' or 'RArm' ");
  }
  return eJe;
}

/*!
  Get Homogeneous matrix cMe: from the camera to the last joint of the chain (HeadRoll).

  \return Homogeneous matrix cMe: from the camera to the last joint of the chain (HeadRoll).

  \param endEffectorName : Name of the camera.

*/
vpHomogeneousMatrix vpNaoqiRobot::get_cMe(const std::string &endEffectorName)
{
  vpHomogeneousMatrix cMe;

  // Transformation matrix from CameraLeft to HeadRoll
  if (endEffectorName == "CameraLeft")
  {
    cMe[0][0] = -1.;
    cMe[1][0] = 0.;
    cMe[2][0] =  0.;

    cMe[0][1] = 0.;
    cMe[1][1] = -1.;
    cMe[2][1] =  0.;

    cMe[0][2] = 0.;
    cMe[1][2] = 0.;
    cMe[2][2] =  1.;

    cMe[0][3] = 0.04;
    cMe[1][3] = 0.09938;
    cMe[2][3] = -0.11999;
  }

  else if (endEffectorName == "CameraLeft_aldebaran")
  {

    vpHomogeneousMatrix eMc_ald;
    eMc_ald[0][3] = 0.11999;
    eMc_ald[1][3] = 0.04;
    eMc_ald[2][3] = 0.09938;

    vpHomogeneousMatrix cam_alMe_camvisp;
    for(unsigned int i=0; i<3; i++)
      cam_alMe_camvisp[i][i] = 0; // remove identity
    cam_alMe_camvisp[0][2] = 1.;
    cam_alMe_camvisp[1][0] = -1.;
    cam_alMe_camvisp[2][1] = -1.;

    cMe = (eMc_ald * cam_alMe_camvisp).inverse();

  }

  else
  {
    throw vpRobotException (vpRobotException::readingParametersError,
                            "Transformation matrix that you requested is not implemented. Valid values: CameraLeft.");
  }
  return cMe;
}
