/**************************************************************************************************
  Filename:       Version.h
  Revised:        $Date: $
  Revision:       $Revision:  $

  Description:    Firmware version file header


 
**************************************************************************************************/

#ifndef VERSION_H
#define VERSION_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */
#define BUILD_VERSION_SIZE  4
#define BUILD_DATE_SIZE     22
#define BOARD_NAME_SIZE     4

/*********************************************************************
 * MACROS
 */
  
/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * VARIABLES
 */
extern const char app_name[];
extern const char board_name[];
//extern const uint8_t build_version[BUILD_VERSION_SIZE];
//extern const char build_date[];
extern const char build_author[];

extern const char build_version[];
extern const char build_date[];
extern const char build_sha[];

/*********************************************************************
 * FUNCTIONS
 */

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* VERSION_H */
