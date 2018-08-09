/* Automatically generated nanopb constant definitions */
/* Generated by nanopb-0.3.9.1 at Wed Aug  1 18:07:08 2018. */

#include "strip.pb.h"

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif

const Strip_Animation Strip_animation_default = Strip_Animation_NONE;


const pb_field_t Strip_fields[5] = {
    PB_FIELD(  1, INT32   , OPTIONAL, STATIC  , FIRST, Strip, brt, brt, 0),
    PB_FIELD(  2, MESSAGE , OPTIONAL, STATIC  , OTHER, Strip, whole, brt, &Strip_Whole_Strip_fields),
    PB_FIELD(  3, MESSAGE , REPEATED, CALLBACK, OTHER, Strip, leds, whole, &Strip_Led_fields),
    PB_FIELD(  4, UENUM   , OPTIONAL, STATIC  , OTHER, Strip, animation, leds, &Strip_animation_default),
    PB_LAST_FIELD
};

const pb_field_t Strip_Led_fields[3] = {
    PB_FIELD(  1, INT32   , REQUIRED, STATIC  , FIRST, Strip_Led, num, num, 0),
    PB_FIELD(  2, FIXED32 , REQUIRED, STATIC  , OTHER, Strip_Led, rgbw, num, 0),
    PB_LAST_FIELD
};

const pb_field_t Strip_Whole_Strip_fields[2] = {
    PB_FIELD(  1, FIXED32 , REQUIRED, STATIC  , FIRST, Strip_Whole_Strip, rgbw, rgbw, 0),
    PB_LAST_FIELD
};



/* Check that field information fits in pb_field_t */
#if !defined(PB_FIELD_32BIT)
/* If you get an error here, it means that you need to define PB_FIELD_32BIT
 * compile-time option. You can do that in pb.h or on compiler command line.
 * 
 * The reason you need to do this is that some of your messages contain tag
 * numbers or field sizes that are larger than what can fit in 8 or 16 bit
 * field descriptors.
 */
PB_STATIC_ASSERT((pb_membersize(Strip, whole) < 65536), YOU_MUST_DEFINE_PB_FIELD_32BIT_FOR_MESSAGES_Strip_Strip_Led_Strip_Whole_Strip)
#endif

#if !defined(PB_FIELD_16BIT) && !defined(PB_FIELD_32BIT)
/* If you get an error here, it means that you need to define PB_FIELD_16BIT
 * compile-time option. You can do that in pb.h or on compiler command line.
 * 
 * The reason you need to do this is that some of your messages contain tag
 * numbers or field sizes that are larger than what can fit in the default
 * 8 bit descriptors.
 */
PB_STATIC_ASSERT((pb_membersize(Strip, whole) < 256), YOU_MUST_DEFINE_PB_FIELD_16BIT_FOR_MESSAGES_Strip_Strip_Led_Strip_Whole_Strip)
#endif


/* @@protoc_insertion_point(eof) */
