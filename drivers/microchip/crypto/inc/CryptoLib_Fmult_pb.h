//========================================================================
//
//        Copyright (c) Atmel Corporation 2012 All rigths reserved
//                     ATMEL CONFIDENTIAL PROPRIETARY
//
//========================================================================
//                             DISCLAIMER
//
//    Atmel reserves the right to make changes without further notice
//    to any product herein to improve reliability, function or design.
//    Atmel does not assume any liability arising out of the
//    application or use of any product, circuit, or software described
//    herein; neither does it convey any license under its patent rights
//    nor the rights of others. Atmel products are not designed,
//    intended or authorized for use as components in systems intended
//    for surgical implant into the body or other applications intended
//    to support life, or for any other application in which the failure
//    of the Atmel product could create a situation where personal
//    injury or death may occur. Should Buyer purchase or use Atmel
//    products for any such intended or unauthorized application, Buyer
//    shall indemnify and hold Atmel and its officers, employees,
//    subsidiaries, affiliates, and distributors harmless against all
//    claims, costs, damages and expenses, and reasonable attorney fees
//    arising out of, directly or indirectly, any claim of personal
//    injury or death associated with such unintended or unauthorized
//    use, even if such claim alleges that Atmel was negligent
//    regarding the design or manufacture of the part. Atmel and the
//    Atmel logo are registered trademarks of the Atmel corporation.
//========================================================================
//      ATMEL Crypto Library
//------------------------------------------------------------------------
//      File: CryptoLib_Fmult_pb.h
//------------------------------------------------------------------------
//      Description: definition of types for the Cryptographic library
//------------------------------------------------------------------------
#ifndef _CRYPTOLIB_FMULT_PB_INCLUDED
#define _CRYPTOLIB_FMULT_PB_INCLUDED

// Structure definition
typedef struct _CPKCL_fmult {
               nu1       nu1ModBase;
               nu1       nu1CnsBase;
               u2        u2ModLength;

               nu1       nu1XBase;
               nu1       nu1YBase;
               nu1       nu1ZBase;
               nu1       nu1RBase;
               u2        u2XLength;
               u2        u2YLength;
               } _CPKCL_FMULT, *_PCPKCL_FMULT;

// Options definition
#define CPKCL_FMULT_ONLY             MULT_ONLY
#define CPKCL_FMULT_ADD              MULT_ADD
#define CPKCL_FMULT_SUB              MULT_SUB


#endif // _CRYPTOLIB_FMULT_PB_INCLUDED
