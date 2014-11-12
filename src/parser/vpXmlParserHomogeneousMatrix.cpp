/****************************************************************************
 *
 * $Id: vpXmlParserHomogeneousMatrix.cpp 4920 2014-10-09 08:18:30Z fspindle $
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
 * See http://www.irisa.fr/lagadic/visp/visp.html for more information.
 *
 * This software was developed at:
 * INRIA Rennes - Bretagne Atlantique
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * France
 * http://www.irisa.fr/lagadic
 *
 * If you have questions regarding the use of this file, please contact
 * INRIA at visp@inria.fr
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *
 * Description:
 * XML parser to load and save camera intrinsic parameters.
 *
 * Authors:
 * Anthony Saunier
 *
 *****************************************************************************/


/*!
  \file vpXmlParserHomogeneousMatrix.cpp
  \brief Definition of the vpXmlParserHomogeneousMatrix class member functions.
  Class vpXmlParserHomogeneousMatrix allowed to load and save intrinsic camera parameters

*/
#include <visp_naoqi/vpXmlParserHomogeneousMatrix.h>
#ifdef VISP_HAVE_XML2

#include <stdlib.h>
#include <string.h>

#include <visp/vpDebug.h>
#include <visp/vpThetaUVector.h>
/* -------------------------------------------------------------------------- */
/* --- LABEL XML ------------------------------------------------------------ */
/* -------------------------------------------------------------------------- */

#define LABEL_XML_ROOT                               "root"
#define LABEL_XML_M                                  "homogeneous_transformation"
#define LABEL_XML_M_NAME                             "name"
#define LABEL_XML_VALUE                              "values"
#define LABEL_XML_TRANSLATION                        "translation"
#define LABEL_XML_TX                                 "tx"
#define LABEL_XML_TY                                 "ty"
#define LABEL_XML_TZ                                 "tz"
#define LABEL_XML_ROTATION                           "rotation"
#define LABEL_XML_TUX                                "theta_ux"
#define LABEL_XML_TUY                                "theta_uy"
#define LABEL_XML_TUZ                                "theta_uz"

/*!
  Default constructor
*/
vpXmlParserHomogeneousMatrix::vpXmlParserHomogeneousMatrix()
  : vpXmlParser(),
    M(), M_name(), tx(0.0),ty(0.0),tz(0.0),tux(0.0),tuy(0.0),tuz(0.0)
{
}
/*!
  Copy constructor
  \param twinParser : parser object to copy
*/
vpXmlParserHomogeneousMatrix::vpXmlParserHomogeneousMatrix(vpXmlParserHomogeneousMatrix& twinParser)
  : vpXmlParser(twinParser), M(),M_name(), tx(0.0),ty(0.0),tz(0.0),tux(0.0),tuy(0.0),tuz(0.0)

{
  this->M = twinParser.M;
  this->M_name = twinParser.M_name;
  this->tx = twinParser.tx;
  this->ty = twinParser.ty;
  this->tz = twinParser.tz;
  this->tux = twinParser.tux;
  this->tuy = twinParser.tuy;
  this->tuz = twinParser.tuz;
}

/*!
  Copy operator
  \param twinParser : parser object to copy
  \return a copy of the input.
*/
vpXmlParserHomogeneousMatrix&
vpXmlParserHomogeneousMatrix::operator =(const vpXmlParserHomogeneousMatrix& twinParser) {
  this->M = twinParser.M;
  this->M_name = twinParser.M_name;
  this->tx = twinParser.tx;
  this->ty = twinParser.ty;
  this->tz = twinParser.tz;
  this->tux = twinParser.tux;
  this->tuy = twinParser.tuy;
  this->tuz = twinParser.tuz;
  return *this ;
}

/*!
  Parse an xml file to load an homogeneous matrix
  \param M_ : homogeneous matrix to fill.
  \param filename : name of the xml file to parse
  \param M_name_ : name of the homogeneous matrix

  \return error code.
*/
//int
//vpXmlParserHomogeneousMatrix::parse(vpCameraParameters &cam, const char * filename,
//                         const std::string& cam_name,
//                         const vpCameraParameters::vpCameraParametersProjType &projModel,
//                         const unsigned int im_width,
//                         const unsigned int im_height)

int
vpXmlParserHomogeneousMatrix::parse(vpHomogeneousMatrix &M_, const char * filename,
                                    const std::string &M_name_)
{
  xmlDocPtr doc;
  xmlNodePtr node;

  doc = xmlParseFile(filename);
  if (doc == NULL)
  {
   std::cerr << std::endl
   << "ERROR:" << std::endl;
   std::cerr << " I cannot open the file "<< filename << std::endl;

   return SEQUENCE_ERROR;
  }

  node = xmlDocGetRootElement(doc);
  if (node == NULL)
  {
    xmlFreeDoc(doc);
    return SEQUENCE_ERROR;
  }

  int ret = this ->read (doc, node, M_name_);

  M_ = M ;

  xmlFreeDoc(doc);

  return ret;
}

/*!
  Save an homogenous matrix in an xml file.
  \param M : homogenous matrix to save.
  \param filename : name of the xml file to fill.
  \param cam_name : name of the homogenous matrix .

  \return error code.
*/
int
vpXmlParserHomogeneousMatrix::save(const vpHomogeneousMatrix &M, const char * filename,
                                   const std::string& M_name)
{
  xmlDocPtr doc;
  xmlNodePtr node;
  xmlNodePtr nodeCamera = NULL;

  doc = xmlReadFile(filename,NULL,XML_PARSE_NOWARNING + XML_PARSE_NOERROR
                    + XML_PARSE_NOBLANKS);
  if (doc == NULL){
    doc = xmlNewDoc ((xmlChar*)"1.0");
    node = xmlNewNode(NULL,(xmlChar*)LABEL_XML_ROOT);
    xmlDocSetRootElement(doc,node);
    xmlNodePtr node_tmp = xmlNewComment((xmlChar*)
                                        "This file stores homogeneous matrix used\n"
                                        "   in the vpHomogeneousMatrix Class of ViSP available\n"
                                        "   at http://www.irisa.fr/lagadic/visp/visp.html .\n"
                                        "   It can be read with the parse method of\n"
                                        "   the vpXmlParserHomogeneousMatrix class.");
    xmlAddChild(node,node_tmp);
  }

  node = xmlDocGetRootElement(doc);
  if (node == NULL)
  {
    xmlFreeDoc(doc);
    return SEQUENCE_ERROR;
  }

  this->M = M;

  int nbCamera = count(doc, node, M_name);

  //std::cout <<"Result find camera with input name: " << nbCamera << std::endl;
  if( nbCamera > 0){
    //vpCERROR
    std::cout << "There is already an Homogeneous matrixy "<< std::endl
              << "available in the file with the input name: "<< M_name << "."<< std::endl
              << "Please delete it manually from the xml file."<< std::endl;
    xmlFreeDoc(doc);
    return SEQUENCE_ERROR;
  }

  write(node, M_name);

  xmlSaveFormatFile(filename,doc,1);
  xmlFreeDoc(doc);
  std::cout << "Homogeneous matrix '"<< M_name << "' saved in the file named "<< filename << " correctly." << std::endl;

  return SEQUENCE_OK;
}



/*!
  Read camera parameters from a XML file.

  \param doc : XML file.
  \param node : XML tree, pointing on a marker equipement.
  \param M_name_ : name of the camera
  \return error code.
 */
int
vpXmlParserHomogeneousMatrix::read (xmlDocPtr doc, xmlNodePtr node,
                                    const std::string& M_name_)
{
  //    char * val_char;
  vpXmlCodeType prop;

  vpXmlCodeSequenceType back = SEQUENCE_OK;
  int nbCamera = 0;

  for (node = node->xmlChildrenNode; node != NULL;  node = node->next)
  {
    if (node->type != XML_ELEMENT_NODE) continue;
    if (SEQUENCE_OK != str2xmlcode ((char*)(node ->name), prop))
    {
      prop = CODE_XML_OTHER;
      back = SEQUENCE_ERROR;
    }

    if (prop == CODE_XML_M){
      if (SEQUENCE_OK == this->read_camera (doc, node, M_name_))
        nbCamera++;
    }
    else back = SEQUENCE_ERROR;
  }

  if (nbCamera == 0){
    back = SEQUENCE_ERROR;
    vpCERROR << "No Homogeneous matrix is available" << std::endl
             << "with name" << M_name_ <<"." << std::endl;
  }
  else if(nbCamera > 1){
    back = SEQUENCE_ERROR;
    vpCERROR << nbCamera << " There are more Homogeneous matrix"  << std::endl
             << "with the same name : "              << std::endl
             << "precise your choice..."                   << std::endl;
  }

  return back;
}
/*!
  Read camera parameters from a XML file and read if there is already a homogeneous matrix
  with the same name.

  \param doc : XML file.
  \param node : XML tree, pointing on a marker equipement.
  \param M_name : name of the homogeneous matrix.

  \return 1 if there is an homogeneous matrix corresponding with the input name, 0 otherwise.
 */
int
vpXmlParserHomogeneousMatrix::count (xmlDocPtr doc, xmlNodePtr node,
                                     const std::string& M_name)
{
  //    char * val_char;
  vpXmlCodeType prop;
  int nbCamera = 0;

  for (node = node->xmlChildrenNode; node != NULL;  node = node->next)
  {
    if (node->type != XML_ELEMENT_NODE) continue;
    if (SEQUENCE_OK != str2xmlcode ((char*)(node ->name), prop))
    {
      prop = CODE_XML_OTHER;
    }
    /*
    switch (prop)
    {
    case CODE_XML_M:
      if (SEQUENCE_OK == this->read_camera (doc, node, camera_name, projModel,
          image_width, image_height,
          subsampling_width, subsampling_height)){
        nbCamera++;
      }
      break;
    default:
      break;
    }
    */
    if (prop== CODE_XML_M) {
      if (SEQUENCE_OK == this->read_camera (doc, node, M_name))
        nbCamera++;
    }
  }

  return nbCamera;
}
/*!
  Read camera headers from a XML file and return the last available
  node pointeur in the xml tree corresponding with inputs.

  \param doc : XML file.
  \param node : XML tree, pointing on a marker equipement.
  \param cam_name : name of the camera : useful if the xml file has multiple
  camera parameters. Set as "" if the camera name is not ambiguous.
  \param im_width : width of image  on which camera calibration was performed.
    Set as 0 if not ambiguous.
  \param im_height : height of the image  on which camera calibration
    was performed. Set as 0 if not ambiguous.
  \param subsampl_width : subsampling of the image width sent by the camera.
    Set as 0 if not ambiguous.
  \param subsampl_height : subsampling of the image height sent by the camera.
    Set as 0 if not ambiguous.

  \return number of available camera parameters corresponding with inputs.
 */
//xmlNodePtr
//vpXmlParserHomogeneousMatrix::find_camera (xmlDocPtr doc, xmlNodePtr node,
//                                           const std::string& cam_name,
//                                           const unsigned int im_width,
//                                           const unsigned int im_height,
//                                           const unsigned int subsampl_width,
//                                           const unsigned int subsampl_height)
//{
//  //    char * val_char;
//  vpXmlCodeType prop;

//  for (node = node->xmlChildrenNode; node != NULL;  node = node->next)
//  {
//    if (node->type != XML_ELEMENT_NODE) continue;
//    if (SEQUENCE_OK != str2xmlcode ((char*)(node ->name), prop))
//    {
//      prop = CODE_XML_OTHER;
//    }
//    /*
//    switch (prop)
//    {
//      case CODE_XML_M:
//        if (SEQUENCE_OK == this->read_camera_header(doc, node, camera_name,
//            image_width, image_height,
//            subsampling_width, subsampling_height)){
//              return node;
//        }
//        break;
//      default:
//        break;
//    }
//    */
//    if(prop == CODE_XML_M){
//      if (SEQUENCE_OK == this->read_camera_header(doc, node, cam_name,
//                                                  im_width, im_height,
//                                                  subsampl_width, subsampl_height))
//        return node;
//    }
//  }
//  return NULL;
//}

/*!
  Read camera fields from a XML file.

  \param doc : XML file.
  \param node : XML tree, pointing on a marker equipement.
  \param cam_name : name of the Homogeneous matrix

  \return error code.

 */
int
vpXmlParserHomogeneousMatrix::read_camera (xmlDocPtr doc, xmlNodePtr node,
                                           const std::string& M_name_)
{
  vpXmlCodeType prop;
  /* read value in the XML file. */
  std::string M_name_tmp = "";
  vpHomogeneousMatrix M_tmp;

  vpXmlCodeSequenceType back = SEQUENCE_OK;

  for (node = node->xmlChildrenNode; node != NULL;  node = node->next)
  {
    // vpDEBUG_TRACE (15, "Carac : %s.", node ->name);
    if (node->type != XML_ELEMENT_NODE) continue;
    if (SEQUENCE_OK != str2xmlcode ((char*)(node ->name), prop))
    {
      prop = CODE_XML_OTHER;
      back = SEQUENCE_ERROR;
    }


    switch (prop)
    {
    case CODE_XML_M_NAME: {
      char * val_char = xmlReadCharChild(doc, node);
      M_name_tmp = val_char;
      xmlFree(val_char);
      break;
    }

    case CODE_XML_VALUE: //VALUE
      if (M_name_ == M_name_tmp)
      {
        std::cout << "Found camera with name: \"" << M_name_tmp << "\"" << std::endl;
        back = read_camera_model(doc, node, M_tmp);
      }
      break;

    case CODE_XML_BAD:
    case CODE_XML_OTHER:
    case CODE_XML_M:
    case CODE_XML_TX:
    case CODE_XML_TY:
    case CODE_XML_TZ:
    case CODE_XML_TUX:
    case CODE_XML_TUY:
    case CODE_XML_TUZ:

    default:
      back = SEQUENCE_ERROR;
      break;
    }

  }

  if( !(M_name_ == M_name_tmp)){
    back = SEQUENCE_ERROR;
  }
  else{
    this-> M = M_tmp;
    //std::cout << "Convert in Homogeneous Matrix:"<< std::endl;
    //std::cout << this-> M << std::endl;
    this-> M_name = M_name_tmp;

  }
  return back;
}
/*!
  Read camera header fields from a XML file.

  \param doc : XML file.
  \param node : XML tree, pointing on a marker equipement.
  \param cam_name : name of the camera : useful if the xml file has multiple
  camera parameters. Set as "" if the camera name is not ambiguous.
  \param im_width : width of image  on which camera calibration was performed.
    Set as 0 if not ambiguous.
  \param im_height : height of the image  on which camera calibration
    was performed. Set as 0 if not ambiguous.
  \param subsampl_width : scale of the image width sent by the camera.
    Set as 0 if not ambiguous.
  \param subsampl_height : scale of the image height sent by the camera.
    Set as 0 if not ambiguous.

  \return error code.

 */
//int
//vpXmlParserHomogeneousMatrix::
//read_camera_header (xmlDocPtr doc, xmlNodePtr node,
//                    const std::string& cam_name,
//                    const unsigned int im_width,
//                    const unsigned int im_height,
//                    const unsigned int subsampl_width,
//                    const unsigned int subsampl_height)
//{
//  vpXmlCodeType prop;
//  /* read value in the XML file. */
//  std::string camera_name_tmp = "";
//  unsigned int image_height_tmp = 0 ;
//  unsigned int image_width_tmp = 0 ;
//  unsigned int subsampling_width_tmp = 0;
//  unsigned int subsampling_height_tmp = 0;
//  //   unsigned int full_width_tmp = 0;
//  //   unsigned int full_height_tmp = 0;
//  vpXmlCodeSequenceType back = SEQUENCE_OK;

//  for (node = node->xmlChildrenNode; node != NULL;  node = node->next)
//  {
//    // vpDEBUG_TRACE (15, "Carac : %s.", node ->name);
//    if (node->type != XML_ELEMENT_NODE) continue;
//    if (SEQUENCE_OK != str2xmlcode ((char*)(node ->name), prop))
//    {
//      prop = CODE_XML_OTHER;
//      back = SEQUENCE_ERROR;
//    }


//    switch (prop)
//    {
//    case CODE_XML_M_NAME:{
//      char * val_char = xmlReadCharChild(doc, node);
//      camera_name_tmp = val_char;
//      xmlFree(val_char);
//    }break;

//    case CODE_XML_WIDTH:
//      image_width_tmp = xmlReadUnsignedIntChild(doc, node);
//      break;

//    case CODE_XML_HEIGHT:
//      image_height_tmp = xmlReadUnsignedIntChild(doc, node);
//      break;
//    case CODE_XML_SUBSAMPLING_WIDTH:
//      subsampling_width_tmp = xmlReadUnsignedIntChild(doc, node);
//      break;
//    case CODE_XML_SUBSAMPLING_HEIGHT:
//      subsampling_height_tmp = xmlReadUnsignedIntChild(doc, node);
//      break;
//      //       case CODE_XML_FULL_WIDTH:
//      //         full_width_tmp = xmlReadUnsignedIntChild(doc, node);
//      //         break;

//      //       case CODE_XML_FULL_HEIGHT:
//      //         full_height_tmp = xmlReadUnsignedIntChild(doc, node);
//      //         break;

//    case CODE_XML_MODEL:
//      break;

//    case CODE_XML_BAD:
//    case CODE_XML_OTHER:
//    case CODE_XML_M:
//    case CODE_XML_FULL_HEIGHT:
//    case CODE_XML_FULL_WIDTH:
//    case CODE_XML_MODEL_TYPE:
//    case CODE_XML_U0:
//    case CODE_XML_V0:
//    case CODE_XML_PX:
//    case CODE_XML_PY:
//    case CODE_XML_KUD:
//    case CODE_XML_KDU:
//    default:
//      back = SEQUENCE_ERROR;
//      break;
//    }
//  }
//  if( !((cam_name == camera_name_tmp) &&
//        (im_width == image_width_tmp || im_width == 0) &&
//        (im_height == image_height_tmp || im_height == 0) &&
//        (subsampl_width == subsampling_width_tmp ||
//         subsampl_width == 0)&&
//        (subsampl_height == subsampling_height_tmp ||
//         subsampl_height == 0))){
//    back = SEQUENCE_ERROR;
//  }
//  return back;
//}

/*!
  Read homogeneous matrix fields from a XML file.

  \param doc : XML file.
  \param node : XML tree, pointing on a marker equipement.
  \param M_tmp : homogeneous matrix to fill with read data (output).

  \return error code.

 */
vpXmlParserHomogeneousMatrix::vpXmlCodeSequenceType
vpXmlParserHomogeneousMatrix::read_camera_model (xmlDocPtr doc, xmlNodePtr node,
                                                 vpHomogeneousMatrix &M_tmp)
{
  // counter of the number of read parameters
  int nb = 0;
  vpXmlCodeType prop;
  /* read value in the XML file. */

  double tx_;
  double ty_;
  double tz_;
  double tux_;
  double tuy_;
  double tuz_;


  vpXmlCodeSequenceType back = SEQUENCE_OK;
  //int validation = 0;

  for (node = node->xmlChildrenNode; node != NULL;  node = node->next)
  {
    // vpDEBUG_TRACE (15, "Carac : %s.", node ->name);
    if (node->type != XML_ELEMENT_NODE) continue;
    if (SEQUENCE_OK != str2xmlcode ((char*)(node ->name), prop))
    {
      prop = CODE_XML_OTHER;
      back = SEQUENCE_ERROR;
    }

    switch (prop)
    {

    case CODE_XML_TX:
      tx_ = xmlReadDoubleChild(doc, node);
      nb++;
      break;
    case CODE_XML_TY:
      ty_ = xmlReadDoubleChild(doc, node);
      nb++;
      break;
    case CODE_XML_TZ:
      tz_ = xmlReadDoubleChild(doc, node);
      nb++;
      break;
    case CODE_XML_TUX:
      tux_ = xmlReadDoubleChild(doc, node);
      nb++;
      break;
    case CODE_XML_TUY:
      tuy_ = xmlReadDoubleChild(doc, node);
      nb++;
      break;
    case CODE_XML_TUZ:
      tuz_ = xmlReadDoubleChild(doc, node);
      nb++;
      break;

    case CODE_XML_BAD:
    case CODE_XML_OTHER:
    case CODE_XML_M:
    case CODE_XML_M_NAME:
    case CODE_XML_VALUE:

    default:
      back = SEQUENCE_ERROR;
      break;
    }
  }

  if (nb != 6)
  {
    vpCERROR <<"ERROR in 'model' field:\n";
    vpCERROR << "it must contain 6 parameters\n";

    return SEQUENCE_ERROR;
  }

  // Create the Homogeneous matrix
  M_tmp.buildFrom(tx_,ty_,tz_,tux_,tuy_,tuz_);

//  std::cout << "Read values from file:" << std::endl;
//  std::cout << "tx:" << tx_<< std::endl;
//  std::cout << "ty:" << ty_<< std::endl;
//  std::cout << "tz:" << tz_<< std::endl;
//  std::cout << "tux:" << tux_<< std::endl;
//  std::cout << "tuy:" << tuy_<< std::endl;
//  std::cout << "tuz:" << tuz_<< std::endl;


  //  }
  //  else if( !strcmp(model_type,LABEL_XML_MODEL_WITH_DISTORTION)){
  //    if (nb != 7 || validation != 0x7F)
  //    {
  //      vpCERROR <<"ERROR in 'model' field:\n";
  //      vpCERROR << "it must contain 7 parameters\n";
  //      xmlFree(model_type);

  //      return SEQUENCE_ERROR;
  //    }
  //    cam_tmp.initPersProjWithDistortion(px,py,u0,v0,kud,kdu);
  //  }
  //  else{
  //    vpERROR_TRACE("projection model type doesn't match with any known model !");
  //    xmlFree(model_type);

  //    return SEQUENCE_ERROR;
  //  }
  //  xmlFree(model_type);

  return back;
}

/*!
  Write Homogeneous Matrix in an XML Tree.

  \param node : XML tree, pointing on a marker equipement.
  \param M_name : name of the Homogeneous Matrix.


  \return error code.
 */
int vpXmlParserHomogeneousMatrix::
write (xmlNodePtr node, const std::string& M_name)
{
  int back = SEQUENCE_OK;

  xmlNodePtr node_tmp;
  xmlNodePtr node_camera;
  xmlNodePtr node_values;
  char str[11];

  // Convert from Rotational matrix to Theta-U vector
  vpRotationMatrix R;
  M.extract(R);

  vpThetaUVector tu(R);


  // <homogenous_transformation>
  node_tmp = xmlNewComment((xmlChar*)" Homogeneous Matrix");
  xmlAddChild(node,node_tmp);
  node_camera = xmlNewNode(NULL,(xmlChar*)LABEL_XML_M);
  xmlAddChild(node,node_camera);
  {
    //<name>

    if(!M_name.empty()){
      node_tmp = xmlNewComment((xmlChar*)"Name of the homogeneous matrix");
      xmlAddChild(node_camera,node_tmp);
      xmlNewTextChild(node_camera,NULL,(xmlChar*)LABEL_XML_M_NAME, (xmlChar*)M_name.c_str());
    }

    //<values>

    node_values = xmlNewNode(NULL,(xmlChar*)LABEL_XML_VALUE);
    xmlAddChild(node_camera,node_values);
    {
      node_tmp = xmlNewComment((xmlChar*)"Translation vector");
      xmlAddChild(node_values,node_tmp);

      //<tx>
      sprintf(str,"%f",M[0][3]);
      xmlNewTextChild(node_values,NULL,(xmlChar*)LABEL_XML_TX,(xmlChar*)str);

      //<ty>
      sprintf(str,"%f",M[1][3]);
      xmlNewTextChild(node_values,NULL,(xmlChar*)LABEL_XML_TY,(xmlChar*)str);

      //<tz>
      sprintf(str,"%f",M[2][3]);
      xmlNewTextChild(node_values,NULL,(xmlChar*)LABEL_XML_TZ,(xmlChar*)str);

      node_tmp = xmlNewComment((xmlChar*)"Rotational vector expressed in angle axis representation");
      xmlAddChild(node_values,node_tmp);

      //<tux>
      sprintf(str,"%f",tu[0]);
      xmlNewTextChild(node_values,NULL,(xmlChar*)LABEL_XML_TUX,(xmlChar*)str);

      //<tuy>
      sprintf(str,"%f",tu[1]);
      xmlNewTextChild(node_values,NULL,(xmlChar*)LABEL_XML_TUY,(xmlChar*)str);

      //<tuz>
      sprintf(str,"%f",tu[2]);
      xmlNewTextChild(node_values,NULL,(xmlChar*)LABEL_XML_TUZ,(xmlChar*)str);

    }

  }
  return back;
}

/*!
  Translate a string (label) to a xml code.
  \param str : string to translate.
  \param res : resulting code.

  \return error code.
*/

vpXmlParserHomogeneousMatrix::vpXmlCodeSequenceType
vpXmlParserHomogeneousMatrix::str2xmlcode (char * str, vpXmlCodeType & res)
{
  vpXmlCodeType val_int = CODE_XML_BAD;
  vpXmlCodeSequenceType back = vpXmlParserHomogeneousMatrix::SEQUENCE_OK;

  // DEBUG_TRACE (9, "# Entree :str=%s.", str);

  if (! strcmp (str,  LABEL_XML_M))
  {
    val_int = CODE_XML_M;
  }
  else if (! strcmp (str,  LABEL_XML_M_NAME))
  {
    val_int = CODE_XML_M_NAME;
  }
  else if (! strcmp (str,  LABEL_XML_VALUE))
  {
    val_int = CODE_XML_VALUE;
  }
  else if (! strcmp (str,  LABEL_XML_TX))
  {
    val_int = CODE_XML_TX;
  }
  else if (! strcmp (str,  LABEL_XML_TY))
  {
    val_int = CODE_XML_TY;
  }
  else if (! strcmp (str,  LABEL_XML_TZ))
  {
    val_int = CODE_XML_TZ;
  }
  else if (! strcmp (str,  LABEL_XML_TUX))
  {
    val_int = CODE_XML_TUX;
  }
  else if (! strcmp (str,  LABEL_XML_TUY))
  {
    val_int = CODE_XML_TUY;
  }
  else if (! strcmp (str,  LABEL_XML_TUZ))
  {
    val_int = CODE_XML_TUZ;
  }


  //  else if (! strcmp (str,  LABEL_XML_MODEL))
  //  {
  //    val_int = CODE_XML_MODEL;
  //  }
  //  else if (! strcmp (str,  LABEL_XML_MODEL_TYPE))
  //  {
  //    val_int = CODE_XML_MODEL_TYPE;
  //  }
  //  else if (! strcmp (str,  LABEL_XML_WIDTH))
  //  {
  //    val_int = CODE_XML_WIDTH;
  //  }
  //  else if (! strcmp (str,  LABEL_XML_HEIGHT))
  //  {
  //    val_int = CODE_XML_HEIGHT;
  //  }
  //  else if (! strcmp (str,  LABEL_XML_SUBSAMPLING_WIDTH))
  //  {
  //    val_int = CODE_XML_SUBSAMPLING_WIDTH;
  //  }
  //  else if (! strcmp (str,  LABEL_XML_SUBSAMPLING_HEIGHT))
  //  {
  //    val_int = CODE_XML_SUBSAMPLING_HEIGHT;
  //  }
  //  else if (! strcmp (str,  LABEL_XML_FULL_WIDTH))
  //  {
  //    val_int = CODE_XML_FULL_WIDTH;
  //  }
  //  else if (! strcmp (str,  LABEL_XML_FULL_HEIGHT))
  //  {
  //    val_int = CODE_XML_FULL_HEIGHT;
  //  }
  //  else if (! strcmp (str,  LABEL_XML_U0))
  //  {
  //    val_int = CODE_XML_U0;
  //  }
  //  else if (! strcmp (str,  LABEL_XML_V0))
  //  {
  //    val_int = CODE_XML_V0;
  //  }
  //  else if (! strcmp (str,  LABEL_XML_PX))
  //  {
  //    val_int = CODE_XML_PX;
  //  }
  //  else if (! strcmp (str,  LABEL_XML_PY))
  //  {
  //    val_int = CODE_XML_PY;
  //  }
  //  else if (! strcmp (str,  LABEL_XML_KUD))
  //  {
  //    val_int = CODE_XML_KUD;
  //  }
  //  else if (! strcmp (str,  LABEL_XML_KDU))
  //  {
  //    val_int = CODE_XML_KDU;
  //  }
  else
  {
    val_int = CODE_XML_OTHER;
  }
  res = val_int;

  return back;
}
#endif //VISP_HAVE_XML2
