/**************************************************************************************************
  Filename:       Version.c
  Revised:        $Date: $
  Revision:       $Revision:  $

  Description:    Firmware version file


 
**************************************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include <Version.h>
/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */ 

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
const char app_name[] = "Blinky";
const char board_name[] = "Ocelot";
//const unsigned char build_version[BUILD_VERSION_SIZE] = {0, 0, 0, 1};
//const char build_date[] = __DATE__ " " __TIME__;
const char build_author[] = BUILD_USER_NAME;
const char build_version[] = "v0.0.1";
const char build_date[] = "2018-06-21 13:09:28";
const char build_sha[] = "9916c6e355ef852e3ead7c09879714838ce31973";

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * LOCAL FUNCTIONS DECLARATION
 */

/*********************************************************************
 * PUBLIC FUNCTIONS DEFINITION
 */

/****************************************************************************
****************************************************************************/
