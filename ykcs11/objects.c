/*
 * Copyright (c) 2015-2016 Yubico AB
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "obj_types.h"
#include "objects.h"
#include <ykpiv.h>
#include <string.h>
#include <stdlib.h>
#include "openssl_utils.h"
#include "utils.h"
#include "debug.h"

#define F4 "\x01\x00\x01" // TODO: already define in mechanisms.c. Move
#define PRIME256V1 "\x06\x08\x2a\x86\x48\xce\x3d\x03\x01\x07" // TODO: already define in mechanisms.c. Move
#define SECP384R1 "\x06\x05\x2b\x81\x04\x00\x22" // TODO: already define in mechanisms.c. Move

static CK_RV get_doa(ykcs11_session_t *s, CK_OBJECT_HANDLE obj, CK_ATTRIBUTE_PTR template);
static CK_RV get_coa(ykcs11_session_t *s, CK_OBJECT_HANDLE obj, CK_ATTRIBUTE_PTR template);
static CK_RV get_proa(ykcs11_session_t *s, CK_OBJECT_HANDLE obj, CK_ATTRIBUTE_PTR template);
static CK_RV get_puoa(ykcs11_session_t *s, CK_OBJECT_HANDLE obj, CK_ATTRIBUTE_PTR template);

//TODO: this is mostly a snippet from OpenSC how to give credit?     Less and less so now
/* Must be in order, and one per enumerated PIV_OBJ */
static piv_obj_t piv_objects[] = {
  {PIV_DATA_OBJ_X509_PIV_AUTH, 1, 0, 0, "X.509 Certificate for PIV Authentication", 0, 0, get_doa, 1},
  {PIV_DATA_OBJ_X509_DS, 1, 0, 0, "X.509 Certificate for Digital Signature", 0, 0, get_doa, 2},
  {PIV_DATA_OBJ_X509_KM, 1, 0, 0, "X.509 Certificate for Key Management", 0, 0, get_doa, 3},
  {PIV_DATA_OBJ_X509_CARD_AUTH, 1, 0, 0, "X.509 Certificate for Card Authentication", 0, 0, get_doa, 4},
  {PIV_DATA_OBJ_X509_RETIRED1, 1, 0, 0, "X.509 Certificate for Retired Key 1", 0, 0, get_doa, 5},
  {PIV_DATA_OBJ_X509_RETIRED2, 1, 0, 0, "X.509 Certificate for Retired Key 2", 0, 0, get_doa, 6},
  {PIV_DATA_OBJ_X509_RETIRED3, 1, 0, 0, "X.509 Certificate for Retired Key 3", 0, 0, get_doa, 7},
  {PIV_DATA_OBJ_X509_RETIRED4, 1, 0, 0, "X.509 Certificate for Retired Key 4", 0, 0, get_doa, 8},
  {PIV_DATA_OBJ_X509_RETIRED5, 1, 0, 0, "X.509 Certificate for Retired Key 5", 0, 0, get_doa, 9},
  {PIV_DATA_OBJ_X509_RETIRED6, 1, 0, 0, "X.509 Certificate for Retired Key 6", 0, 0, get_doa, 10},
  {PIV_DATA_OBJ_X509_RETIRED7, 1, 0, 0, "X.509 Certificate for Retired Key 7", 0, 0, get_doa, 11},
  {PIV_DATA_OBJ_X509_RETIRED8, 1, 0, 0, "X.509 Certificate for Retired Key 8", 0, 0, get_doa, 12},
  {PIV_DATA_OBJ_X509_RETIRED9, 1, 0, 0, "X.509 Certificate for Retired Key 9", 0, 0, get_doa, 13},
  {PIV_DATA_OBJ_X509_RETIRED10, 1, 0, 0, "X.509 Certificate for Retired Key 10", 0, 0, get_doa, 14},
  {PIV_DATA_OBJ_X509_RETIRED11, 1, 0, 0, "X.509 Certificate for Retired Key 11", 0, 0, get_doa, 15},
  {PIV_DATA_OBJ_X509_RETIRED12, 1, 0, 0, "X.509 Certificate for Retired Key 12", 0, 0, get_doa, 16},
  {PIV_DATA_OBJ_X509_RETIRED13, 1, 0, 0, "X.509 Certificate for Retired Key 13", 0, 0, get_doa, 17},
  {PIV_DATA_OBJ_X509_RETIRED14, 1, 0, 0, "X.509 Certificate for Retired Key 14", 0, 0, get_doa, 18},
  {PIV_DATA_OBJ_X509_RETIRED15, 1, 0, 0, "X.509 Certificate for Retired Key 15", 0, 0, get_doa, 19},
  {PIV_DATA_OBJ_X509_RETIRED16, 1, 0, 0, "X.509 Certificate for Retired Key 16", 0, 0, get_doa, 20},
  {PIV_DATA_OBJ_X509_RETIRED17, 1, 0, 0, "X.509 Certificate for Retired Key 17", 0, 0, get_doa, 21},
  {PIV_DATA_OBJ_X509_RETIRED18, 1, 0, 0, "X.509 Certificate for Retired Key 18", 0, 0, get_doa, 22},
  {PIV_DATA_OBJ_X509_RETIRED19, 1, 0, 0, "X.509 Certificate for Retired Key 19", 0, 0, get_doa, 23},
  {PIV_DATA_OBJ_X509_RETIRED20, 1, 0, 0, "X.509 Certificate for Retired Key 20", 0, 0, get_doa, 24},
  {PIV_DATA_OBJ_X509_ATTESTATION, 1, 0, 0, "X.509 Certificate for Attestation", 0, 0, get_doa, 25},

  {PIV_DATA_OBJ_CCC, 1, 0, 0, "Card Capability Container", 0, 0, get_doa, 26},
  {PIV_DATA_OBJ_CHUI, 1, 0, 0, "Card Holder Unique Identifier", 0, 0, get_doa, 27},
  {PIV_DATA_OBJ_CHF, 1, 1, 0, "Card Holder Fingerprints", 0, 0, get_doa, 28},
  {PIV_DATA_OBJ_SEC_OBJ, 1, 0, 0, "Security Object", 0, 0, get_doa, 29},
  {PIV_DATA_OBJ_CHFI, 1, 1, 0, "Cardholder Facial Images", 0, 0, get_doa, 30},
  {PIV_DATA_OBJ_PI, 1, 1, 0, "Printed Information", 0, 0, get_doa, 31},
  {PIV_DATA_OBJ_DISCOVERY, 1, 0, 0, "Discovery Object", 0, 0, get_doa, 32},
  {PIV_DATA_OBJ_HISTORY, 1, 0, 0, "Key History Object", 0, 0, get_doa, 33},
  {PIV_DATA_OBJ_IRIS_IMAGE, 1, 1, 0, "Cardholder Iris Images", 0, 0, get_doa, 34},
  {PIV_DATA_OBJ_BITGT, 1, 0, 0, "Biometric Information Templates Group Template", 0, 0, get_doa, 35},
  {PIV_DATA_OBJ_SM_SIGNER, 1, 0, 0, "Secure Messaging Certificate Signer", 0, 0, get_doa, 36},
  {PIV_DATA_OBJ_PC_REF_DATA, 1, 1, 0, "Pairing Code Reference Data Container", 0, 0, get_doa, 37},

  {PIV_CERT_OBJ_X509_PIV_AUTH, 1, 0, 0, "X.509 Certificate for PIV Authentication", 0, 0, get_coa, 1},
  {PIV_CERT_OBJ_X509_DS, 1, 0, 0, "X.509 Certificate for Digital Signature", 0, 0, get_coa, 2},
  {PIV_CERT_OBJ_X509_KM, 1, 0, 0, "X.509 Certificate for Key Management", 0, 0, get_coa, 3},
  {PIV_CERT_OBJ_X509_CARD_AUTH, 1, 0, 0, "X.509 Certificate for Card Authentication", 0, 0, get_coa, 4},
  {PIV_CERT_OBJ_X509_RETIRED1, 1, 0, 0, "X.509 Certificate for Retired Key 1", 0, 0, get_coa, 5},
  {PIV_CERT_OBJ_X509_RETIRED2, 1, 0, 0, "X.509 Certificate for Retired Key 2", 0, 0, get_coa, 6},
  {PIV_CERT_OBJ_X509_RETIRED3, 1, 0, 0, "X.509 Certificate for Retired Key 3", 0, 0, get_coa, 7},
  {PIV_CERT_OBJ_X509_RETIRED4, 1, 0, 0, "X.509 Certificate for Retired Key 4", 0, 0, get_coa, 8},
  {PIV_CERT_OBJ_X509_RETIRED5, 1, 0, 0, "X.509 Certificate for Retired Key 5", 0, 0, get_coa, 9},
  {PIV_CERT_OBJ_X509_RETIRED6, 1, 0, 0, "X.509 Certificate for Retired Key 6", 0, 0, get_coa, 10},
  {PIV_CERT_OBJ_X509_RETIRED7, 1, 0, 0, "X.509 Certificate for Retired Key 7", 0, 0, get_coa, 11},
  {PIV_CERT_OBJ_X509_RETIRED8, 1, 0, 0, "X.509 Certificate for Retired Key 8", 0, 0, get_coa, 12},
  {PIV_CERT_OBJ_X509_RETIRED9, 1, 0, 0, "X.509 Certificate for Retired Key 9", 0, 0, get_coa, 13},
  {PIV_CERT_OBJ_X509_RETIRED10, 1, 0, 0, "X.509 Certificate for Retired Key 10", 0, 0, get_coa, 14},
  {PIV_CERT_OBJ_X509_RETIRED11, 1, 0, 0, "X.509 Certificate for Retired Key 11", 0, 0, get_coa, 15},
  {PIV_CERT_OBJ_X509_RETIRED12, 1, 0, 0, "X.509 Certificate for Retired Key 12", 0, 0, get_coa, 16},
  {PIV_CERT_OBJ_X509_RETIRED13, 1, 0, 0, "X.509 Certificate for Retired Key 13", 0, 0, get_coa, 17},
  {PIV_CERT_OBJ_X509_RETIRED14, 1, 0, 0, "X.509 Certificate for Retired Key 14", 0, 0, get_coa, 18},
  {PIV_CERT_OBJ_X509_RETIRED15, 1, 0, 0, "X.509 Certificate for Retired Key 15", 0, 0, get_coa, 19},
  {PIV_CERT_OBJ_X509_RETIRED16, 1, 0, 0, "X.509 Certificate for Retired Key 16", 0, 0, get_coa, 20},
  {PIV_CERT_OBJ_X509_RETIRED17, 1, 0, 0, "X.509 Certificate for Retired Key 17", 0, 0, get_coa, 21},
  {PIV_CERT_OBJ_X509_RETIRED18, 1, 0, 0, "X.509 Certificate for Retired Key 18", 0, 0, get_coa, 22},
  {PIV_CERT_OBJ_X509_RETIRED19, 1, 0, 0, "X.509 Certificate for Retired Key 19", 0, 0, get_coa, 23},
  {PIV_CERT_OBJ_X509_RETIRED20, 1, 0, 0, "X.509 Certificate for Retired Key 20", 0, 0, get_coa, 24},
  {PIV_CERT_OBJ_X509_ATTESTATION, 1, 0, 0, "X.509 Certificate for Attestation", 0, 0, get_coa, 25},

  {PIV_PVTK_OBJ_PIV_AUTH, 1, 1, 0, "Private key for PIV Authentication", 0, 0, get_proa, 1},   // 9a
  {PIV_PVTK_OBJ_DS, 1, 1, 0, "Private key for Digital Signature", 0, 0, get_proa, 2},          // 9c
  {PIV_PVTK_OBJ_KM, 1, 1, 0, "Private key for Key Management", 0, 0, get_proa, 3},             // 9d
  {PIV_PVTK_OBJ_CARD_AUTH, 1, 1, 0, "Private key for Card Authentication", 0, 0, get_proa, 4}, // 9e
  {PIV_PVTK_OBJ_RETIRED1, 1, 1, 0, "Private key for Retired Key 1", 0, 0, get_proa, 5},
  {PIV_PVTK_OBJ_RETIRED2, 1, 1, 0, "Private key for Retired Key 2", 0, 0, get_proa, 6},
  {PIV_PVTK_OBJ_RETIRED3, 1, 1, 0, "Private key for Retired Key 3", 0, 0, get_proa, 7},
  {PIV_PVTK_OBJ_RETIRED4, 1, 1, 0, "Private key for Retired Key 4", 0, 0, get_proa, 8},
  {PIV_PVTK_OBJ_RETIRED5, 1, 1, 0, "Private key for Retired Key 5", 0, 0, get_proa, 9},
  {PIV_PVTK_OBJ_RETIRED6, 1, 1, 0, "Private key for Retired Key 6", 0, 0, get_proa, 10},
  {PIV_PVTK_OBJ_RETIRED7, 1, 1, 0, "Private key for Retired Key 7", 0, 0, get_proa, 11},
  {PIV_PVTK_OBJ_RETIRED8, 1, 1, 0, "Private key for Retired Key 8", 0, 0, get_proa, 12},
  {PIV_PVTK_OBJ_RETIRED9, 1, 1, 0, "Private key for Retired Key 9", 0, 0, get_proa, 13},
  {PIV_PVTK_OBJ_RETIRED10, 1, 1, 0, "Private key for Retired Key 10", 0, 0, get_proa, 14},
  {PIV_PVTK_OBJ_RETIRED11, 1, 1, 0, "Private key for Retired Key 11", 0, 0, get_proa, 15},
  {PIV_PVTK_OBJ_RETIRED12, 1, 1, 0, "Private key for Retired Key 12", 0, 0, get_proa, 16},
  {PIV_PVTK_OBJ_RETIRED13, 1, 1, 0, "Private key for Retired Key 13", 0, 0, get_proa, 17},
  {PIV_PVTK_OBJ_RETIRED14, 1, 1, 0, "Private key for Retired Key 14", 0, 0, get_proa, 18},
  {PIV_PVTK_OBJ_RETIRED15, 1, 1, 0, "Private key for Retired Key 15", 0, 0, get_proa, 19},
  {PIV_PVTK_OBJ_RETIRED16, 1, 1, 0, "Private key for Retired Key 16", 0, 0, get_proa, 20},
  {PIV_PVTK_OBJ_RETIRED17, 1, 1, 0, "Private key for Retired Key 17", 0, 0, get_proa, 21},
  {PIV_PVTK_OBJ_RETIRED18, 1, 1, 0, "Private key for Retired Key 18", 0, 0, get_proa, 22},
  {PIV_PVTK_OBJ_RETIRED19, 1, 1, 0, "Private key for Retired Key 19", 0, 0, get_proa, 23},
  {PIV_PVTK_OBJ_RETIRED20, 1, 1, 0, "Private key for Retired Key 20", 0, 0, get_proa, 24},
  {PIV_PVTK_OBJ_ATTESTATION, 1, 1, 0, "Private key for Attestation", 0, 0, get_proa, 25},

  {PIV_PUBK_OBJ_PIV_AUTH, 1, 0, 0, "Public key for PIV Authentication", 0, 0, get_puoa, 1},
  {PIV_PUBK_OBJ_DS, 1, 0, 0, "Public key for Digital Signature", 0, 0, get_puoa, 2},
  {PIV_PUBK_OBJ_KM, 1, 0, 0, "Public key for Key Management", 0, 0, get_puoa, 3},
  {PIV_PUBK_OBJ_CARD_AUTH, 1, 0, 0, "Public key for Card Authentication", 0, 0, get_puoa, 4},
  {PIV_PUBK_OBJ_RETIRED1, 1, 0, 0, "Public key for Retired Key 1", 0, 0, get_puoa, 5},
  {PIV_PUBK_OBJ_RETIRED2, 1, 0, 0, "Public key for Retired Key 2", 0, 0, get_puoa, 6},
  {PIV_PUBK_OBJ_RETIRED3, 1, 0, 0, "Public key for Retired Key 3", 0, 0, get_puoa, 7},
  {PIV_PUBK_OBJ_RETIRED4, 1, 0, 0, "Public key for Retired Key 4", 0, 0, get_puoa, 8},
  {PIV_PUBK_OBJ_RETIRED5, 1, 0, 0, "Public key for Retired Key 5", 0, 0, get_puoa, 9},
  {PIV_PUBK_OBJ_RETIRED6, 1, 0, 0, "Public key for Retired Key 6", 0, 0, get_puoa, 10},
  {PIV_PUBK_OBJ_RETIRED7, 1, 0, 0, "Public key for Retired Key 7", 0, 0, get_puoa, 11},
  {PIV_PUBK_OBJ_RETIRED8, 1, 0, 0, "Public key for Retired Key 8", 0, 0, get_puoa, 12},
  {PIV_PUBK_OBJ_RETIRED9, 1, 0, 0, "Public key for Retired Key 9", 0, 0, get_puoa, 13},
  {PIV_PUBK_OBJ_RETIRED10, 1, 0, 0, "Public key for Retired Key 10", 0, 0, get_puoa, 14},
  {PIV_PUBK_OBJ_RETIRED11, 1, 0, 0, "Public key for Retired Key 11", 0, 0, get_puoa, 15},
  {PIV_PUBK_OBJ_RETIRED12, 1, 0, 0, "Public key for Retired Key 12", 0, 0, get_puoa, 16},
  {PIV_PUBK_OBJ_RETIRED13, 1, 0, 0, "Public key for Retired Key 13", 0, 0, get_puoa, 17},
  {PIV_PUBK_OBJ_RETIRED14, 1, 0, 0, "Public key for Retired Key 14", 0, 0, get_puoa, 18},
  {PIV_PUBK_OBJ_RETIRED15, 1, 0, 0, "Public key for Retired Key 15", 0, 0, get_puoa, 19},
  {PIV_PUBK_OBJ_RETIRED16, 1, 0, 0, "Public key for Retired Key 16", 0, 0, get_puoa, 20},
  {PIV_PUBK_OBJ_RETIRED17, 1, 0, 0, "Public key for Retired Key 17", 0, 0, get_puoa, 21},
  {PIV_PUBK_OBJ_RETIRED18, 1, 0, 0, "Public key for Retired Key 18", 0, 0, get_puoa, 22},
  {PIV_PUBK_OBJ_RETIRED19, 1, 0, 0, "Public key for Retired Key 19", 0, 0, get_puoa, 23},
  {PIV_PUBK_OBJ_RETIRED20, 1, 0, 0, "Public key for Retired Key 20", 0, 0, get_puoa, 24},
  {PIV_PUBK_OBJ_ATTESTATION, 1, 0, 0, "Public key for Attestation", 0, 0, get_puoa, 25}
};

static piv_data_obj_t data_objects[] = {
  {0, ""},
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x01\x01"},	// 2.16.840.1.101.3.7.2.1.1
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x01\x00"},	// 2.16.840.1.101.3.7.2.1.0
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x01\x02"},	// 2.16.840.1.101.3.7.2.1.2
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x05\x00"},	// 2.16.840.1.101.3.7.2.5.0
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x01"},	// 2.16.840.1.101.3.7.2.16.1
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x02"},	// 2.16.840.1.101.3.7.2.16.2
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x03"},	// 2.16.840.1.101.3.7.2.16.3
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x04"},	// 2.16.840.1.101.3.7.2.16.4
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x05"},	// 2.16.840.1.101.3.7.2.16.5
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x06"},	// 2.16.840.1.101.3.7.2.16.6
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x07"},	// 2.16.840.1.101.3.7.2.16.7
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x08"},	// 2.16.840.1.101.3.7.2.16.8
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x09"},	// 2.16.840.1.101.3.7.2.16.9
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x0a"},	// 2.16.840.1.101.3.7.2.16.10
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x0b"},	// 2.16.840.1.101.3.7.2.16.11
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x0c"},	// 2.16.840.1.101.3.7.2.16.12
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x0d"},	// 2.16.840.1.101.3.7.2.16.13
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x0e"},	// 2.16.840.1.101.3.7.2.16.14
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x0f"},	// 2.16.840.1.101.3.7.2.16.15
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x10"},	// 2.16.840.1.101.3.7.2.16.16
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x11"},	// 2.16.840.1.101.3.7.2.16.17
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x12"},	// 2.16.840.1.101.3.7.2.16.18
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x13"},	// 2.16.840.1.101.3.7.2.16.19
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x14"},	// 2.16.840.1.101.3.7.2.16.20
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x15"},	// 2.16.840.1.101.3.7.2.16.21 TODO: FIXME !!!
	{11, "\x60\x86\x48\x01\x65\x03\x07\x01\x81\x5b\x00"},	// 2.16.840.1.101.3.7.1.219.0
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x30\x00"},	// 2.16.840.1.101.3.7.2.48.0
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x60\x10"},	// 2.16.840.1.101.3.7.2.96.16
	{11, "\x60\x86\x48\x01\x65\x03\x07\x02\x81\x10\x00"},	// 2.16.840.1.101.3.7.2.144.0
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x60\x30"},	// 2.16.840.1.101.3.7.2.96.48
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x30\x01"},	// 2.16.840.1.101.3.7.2.48.1
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x60\x50"},	// 2.16.840.1.101.3.7.2.96.80
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x60\x60"},	// 2.16.840.1.101.3.7.2.96.96
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x15"},	// 2.16.840.1.101.3.7.2.16.21
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x16"},	// 2.16.840.1.101.3.7.2.16.22
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x17"},	// 2.16.840.1.101.3.7.2.16.23
	{10, "\x60\x86\x48\x01\x65\x03\x07\x02\x10\x18"},	// 2.16.840.1.101.3.7.2.16.24
};

static piv_pvtk_obj_t pvtkey_objects[] = {
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 1},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0},
  {1, 1, 0, 0, 0}
};

static piv_pubk_obj_t pubkey_objects[] = {
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0}
};

/* Get data object attribute */
static CK_RV get_doa(ykcs11_session_t *s, CK_OBJECT_HANDLE obj, CK_ATTRIBUTE_PTR template) {
  CK_BYTE_PTR data;
  CK_BYTE     tmp;
  CK_ULONG    ul_tmp;
  CK_ULONG    len = 0;
  CK_RV       rv;
  DBG("For data object %lu, get ", obj);

  switch (template->type) {
  case CKA_CLASS:
    DBG("CLASS");
    len = sizeof(CK_ULONG);
    ul_tmp = CKO_DATA;
    data = (CK_BYTE_PTR)&ul_tmp;
    break;

  case CKA_TOKEN:
    // Technically all these objects are token objects
    DBG("TOKEN");
    len = sizeof(CK_BBOOL);
    tmp = piv_objects[obj].token;
    data = &tmp;
    break;

  case CKA_PRIVATE:
    DBG("PRIVATE");
    len = sizeof(CK_BBOOL);
    tmp = piv_objects[obj].private;
    data = &tmp;
    break;

  case CKA_LABEL:
    DBG("LABEL");
    len = strlen(piv_objects[obj].label);
    data = (CK_BYTE_PTR) piv_objects[obj].label;
    break;

  case CKA_ID:
    DBG("ID");
    len = sizeof(CK_BYTE);
    tmp = piv_objects[obj].sub_id;
    data = &tmp;
    break;

  case CKA_APPLICATION:
    DBG("APPLICATION");
    len = strlen(piv_objects[obj].label);
    data = (CK_BYTE_PTR) piv_objects[obj].label;
    break;

  case CKA_OBJECT_ID:
    DBG("OID");
    len = data_objects[piv_objects[obj].sub_id].len;
    data = (CK_BYTE_PTR)data_objects[piv_objects[obj].sub_id].data;
    break;

  case CKA_MODIFIABLE:
    DBG("MODIFIABLE");
    len = sizeof(CK_BBOOL);
    tmp = piv_objects[obj].modifiable;
    data = &tmp;
    break;

  case CKA_VALUE:
    DBG("VALUE");
    len = s->data[piv_objects[obj].sub_id].len;
    data = s->data[piv_objects[obj].sub_id].data;
    break;

  default:
    DBG("UNKNOWN ATTRIBUTE %lx", template[0].type);
    template->ulValueLen = CK_UNAVAILABLE_INFORMATION;
    return CKR_ATTRIBUTE_TYPE_INVALID;
  }

  /* Just get the length */
  if (template->pValue == NULL_PTR) {
    template->ulValueLen = len;
    return CKR_OK;
  }

  /* Actually get the attribute */
  if (template->ulValueLen < len)
    return CKR_BUFFER_TOO_SMALL;

  template->ulValueLen = len;
  memcpy(template->pValue, data, len);

  return CKR_OK;
}

/* Get certificate object attribute */
static CK_RV get_coa(ykcs11_session_t *s, CK_OBJECT_HANDLE obj, CK_ATTRIBUTE_PTR template) {
  CK_BYTE_PTR data;
  CK_BYTE     b_tmp[3072]; // Max cert value for ykpiv
  CK_ULONG    ul_tmp;
  CK_ULONG    len = 0;
  CK_RV       rv;
  DBG("For certificate object %lu, get ", obj);

  switch (template->type) {
  case CKA_CLASS:
    DBG("CLASS");
    len = sizeof(CK_ULONG);
    ul_tmp = CKO_CERTIFICATE;
    data = (CK_BYTE_PTR) &ul_tmp;
    break;

  case CKA_TOKEN:
    // Technically all these objects are token objects
    DBG("TOKEN");
    len = sizeof(CK_BBOOL);
    b_tmp[0] = piv_objects[obj].token;
    data = b_tmp;
    break;

  case CKA_PRIVATE:
    DBG("PRIVATE");
    len = sizeof(CK_BBOOL);
    b_tmp[0] = piv_objects[obj].private;
    data = b_tmp;
    break;

  case CKA_LABEL:
    DBG("LABEL");
    len = strlen(piv_objects[obj].label);
    data = (CK_BYTE_PTR) piv_objects[obj].label;
    break;

  case CKA_SUBJECT:
    DBG("SUBJECT");
    len = sizeof(b_tmp);
    if ((rv = do_get_raw_name(X509_get_subject_name(s->certs[piv_objects[obj].sub_id]), b_tmp, &len)) != CKR_OK)
      return rv;
    data = b_tmp;
    break;

  case CKA_ISSUER:
    DBG("ISSUER");
    len = sizeof(b_tmp);
    if ((rv = do_get_raw_name(X509_get_issuer_name(s->certs[piv_objects[obj].sub_id]), b_tmp, &len)) != CKR_OK)
      return rv;
    data = b_tmp;
    break;

  case CKA_SERIAL_NUMBER:
    DBG("SERIAL_NUMBER");
    len = sizeof(b_tmp);
    if ((rv = do_get_raw_integer(X509_get_serialNumber(s->certs[piv_objects[obj].sub_id]), b_tmp, &len)) != CKR_OK)
      return rv;
    data = b_tmp;
    break;

  case CKA_VALUE:
    DBG("VALUE");
    len = sizeof(b_tmp);
    if ((rv = do_get_raw_cert(s->certs[piv_objects[obj].sub_id], b_tmp, &len)) != CKR_OK)
      return rv;
    data = b_tmp;
    break;

  case CKA_CERTIFICATE_TYPE:
    DBG("CERTIFICATE TYPE");
    len = sizeof(CK_ULONG);
    ul_tmp = CKC_X_509; // Support only X.509 certs
    data = (CK_BYTE_PTR) &ul_tmp;
    break;

  case CKA_ID:
    DBG("ID");
    len = sizeof(CK_BYTE);
    b_tmp[0] = piv_objects[obj].sub_id;
    data = b_tmp;
    break;

  case CKA_MODIFIABLE:
    DBG("MODIFIABLE");
    len = sizeof(CK_BBOOL);
    b_tmp[0] = piv_objects[obj].modifiable;
    data = b_tmp;
    break;

  case CKA_TRUSTED:
    DBG("TRUSTED");
    len = sizeof(CK_BBOOL);
    b_tmp[0] = 0;
    data = b_tmp;
    break;

  /* case CKA_START_DATE: */
  /* case CKA_END_DATE: */
  default: // TODO: there are other attributes for a (x509) certificate
    DBG("UNKNOWN ATTRIBUTE %lx", template[0].type);
    template->ulValueLen = CK_UNAVAILABLE_INFORMATION;
    return CKR_ATTRIBUTE_TYPE_INVALID;
  }

  /* Just get the length */
  if (template->pValue == NULL_PTR) {
    template->ulValueLen = len;
    return CKR_OK;
  }

  /* Actually get the attribute */
  if (template->ulValueLen < len)
    return CKR_BUFFER_TOO_SMALL;

  template->ulValueLen = len;
  memcpy(template->pValue, data, len);

  return CKR_OK;
}

/* Get private key object attribute */
static CK_RV get_proa(ykcs11_session_t *s, CK_OBJECT_HANDLE obj, CK_ATTRIBUTE_PTR template) {
  CK_BYTE_PTR data;
  CK_BYTE     b_tmp[1024];
  CK_ULONG    ul_tmp; // TODO: fix elsewhere too
  CK_ULONG    len = 0;
  CK_RV       rv;
  DBG("For private key object %lu, get ", obj);

  switch (template->type) {
  case CKA_CLASS:
    DBG("CLASS");
    len = sizeof(CK_ULONG);
    ul_tmp = CKO_PRIVATE_KEY;
    data = (CK_BYTE_PTR) &ul_tmp;
    break;

  case CKA_TOKEN:
    // Technically all these objects are token objects
    DBG("TOKEN");
    len = sizeof(CK_BBOOL);
    b_tmp[0] = piv_objects[obj].token;
    data = b_tmp;
    break;

  case CKA_PRIVATE:
    DBG("PRIVATE");
    len = sizeof(CK_BBOOL);
    b_tmp[0] = piv_objects[obj].private;
    data = b_tmp;
    break;

  case CKA_LABEL:
    DBG("LABEL");
    len = strlen(piv_objects[obj].label);
    data = (CK_BYTE_PTR) piv_objects[obj].label;
    break;

  case CKA_KEY_TYPE:
    DBG("KEY TYPE");
    len = sizeof(CK_ULONG);
    ul_tmp = do_get_key_type(s->pkeys[piv_objects[obj].sub_id]); // Getting the info from the pubk
    if (ul_tmp == CKK_VENDOR_DEFINED)
      return CKR_FUNCTION_FAILED;
    data = (CK_BYTE_PTR) &ul_tmp;
    break;

  case CKA_ID:
    DBG("ID");
    len = sizeof(CK_BYTE);
    ul_tmp = piv_objects[obj].sub_id;
    data = (CK_BYTE_PTR) &ul_tmp;
    break;

  case CKA_SENSITIVE:
    DBG("SENSITIVE"); // Always true
    len = sizeof(CK_BBOOL);
    b_tmp[0] = 1;
    data = b_tmp;
    break;

  case CKA_EXTRACTABLE:
    DBG("EXTRACTABLE"); // Always false
    len = sizeof(CK_BBOOL);
    b_tmp[0] = 0;
    data = b_tmp;
    break;

  case CKA_LOCAL:
    DBG("LOCAL"); // Always true
    len = sizeof(CK_BBOOL);
    b_tmp[0] = 1;
    data = b_tmp;
    break;

  case CKA_DECRYPT:
    DBG("DECRYPT"); // Default empy
    len = sizeof(CK_BBOOL);
    b_tmp[0] = pvtkey_objects[piv_objects[obj].sub_id].decrypt;
    data = b_tmp;
    break;

  case CKA_UNWRAP:
    DBG("UNWRAP"); // Default empty
    len = sizeof(CK_BBOOL);
    b_tmp[0] = pvtkey_objects[piv_objects[obj].sub_id].unwrap;
    data = b_tmp;
    break;

  case CKA_SIGN:
    DBG("SIGN"); // Default empty
    len = sizeof(CK_BBOOL);
    b_tmp[0] = pvtkey_objects[piv_objects[obj].sub_id].sign;
    data = b_tmp;
    break;

  case CKA_DERIVE:
    DBG("DERIVE"); // Default false
    len = sizeof(CK_BBOOL);
    b_tmp[0] = pvtkey_objects[piv_objects[obj].sub_id].derive;
    data = b_tmp;
    break;

  case CKA_MODULUS:
    DBG("MODULUS");
    len = sizeof(b_tmp);

    // Make sure that this is an RSA key
    ul_tmp = do_get_key_type(s->pkeys[piv_objects[obj].sub_id]); // Getting the info from the pubk
    if (ul_tmp == CKK_VENDOR_DEFINED)
      return CKR_FUNCTION_FAILED;
    if (ul_tmp != CKK_RSA) {
      template->ulValueLen = CK_UNAVAILABLE_INFORMATION;
      return CKR_ATTRIBUTE_TYPE_INVALID;
    }

    if ((rv = do_get_modulus(s->pkeys[piv_objects[obj].sub_id], b_tmp, &len)) != CKR_OK)
      return rv;
    data = b_tmp;
    break;

  case CKA_EC_POINT:
    DBG("EC_POINT");
    len = sizeof(b_tmp);

    // Make sure that this is an EC key
    ul_tmp = do_get_key_type(s->pkeys[piv_objects[obj].sub_id]); // Getting the info from the pubk
    if (ul_tmp == CKK_VENDOR_DEFINED)
      return CKR_FUNCTION_FAILED;
    if (ul_tmp != CKK_ECDSA) {
      template->ulValueLen = CK_UNAVAILABLE_INFORMATION;
      return CKR_ATTRIBUTE_TYPE_INVALID;
    }

    if ((rv = do_get_public_key(s->pkeys[piv_objects[obj].sub_id], b_tmp, &len)) != CKR_OK)
      return rv;
    data = b_tmp;
    break;

  case CKA_EC_PARAMS:
    // Here we want the curve parameters (DER encoded OID)
    DBG("EC_PARAMS");
    len = sizeof(b_tmp);

    // Make sure that this is an EC key
    ul_tmp = do_get_key_type(s->pkeys[piv_objects[obj].sub_id]); // Getting the info from the pubk
    if (ul_tmp == CKK_VENDOR_DEFINED)
      return CKR_FUNCTION_FAILED;
    if (ul_tmp != CKK_ECDSA) {
      template->ulValueLen = CK_UNAVAILABLE_INFORMATION;
      return CKR_ATTRIBUTE_TYPE_INVALID;
    }

    if ((rv = do_get_curve_parameters(s->pkeys[piv_objects[obj].sub_id], b_tmp, &len)) != CKR_OK)
      return rv;

    data = b_tmp;
    break;

  case CKA_MODULUS_BITS:
    DBG("MODULUS BITS");
    len = sizeof(CK_ULONG);

    // Make sure that this is an RSA key
    ul_tmp = do_get_key_type(s->pkeys[piv_objects[obj].sub_id]); // Getting the info from the pubk
    if (ul_tmp == CKK_VENDOR_DEFINED)
      return CKR_FUNCTION_FAILED;
    if (ul_tmp != CKK_RSA) {
      template->ulValueLen = CK_UNAVAILABLE_INFORMATION;
      return CKR_ATTRIBUTE_TYPE_INVALID;
    }

    ul_tmp = do_get_rsa_modulus_length(s->pkeys[piv_objects[obj].sub_id]); // Getting the info from the pubk
    if (ul_tmp == 0)
      return CKR_FUNCTION_FAILED;
    data = (CK_BYTE_PTR) &ul_tmp;
    break;

  case CKA_PUBLIC_EXPONENT:
    DBG("PUBLIC EXPONENT");
    len = sizeof(CK_ULONG);

    // Make sure that this is an RSA key
    ul_tmp = do_get_key_type(s->pkeys[piv_objects[obj].sub_id]); // Getting the info from the pubk
    if (ul_tmp == CKK_VENDOR_DEFINED)
      return CKR_FUNCTION_FAILED;
    if (ul_tmp != CKK_RSA) {
      template->ulValueLen = CK_UNAVAILABLE_INFORMATION;
      return CKR_ATTRIBUTE_TYPE_INVALID;
    }

    if ((rv = do_get_public_exponent(s->pkeys[piv_objects[obj].sub_id], b_tmp, &len)) != CKR_OK)
      return rv;
    data = b_tmp;
    break;

  case CKA_ALWAYS_AUTHENTICATE:
    DBG("ALWAYS AUTHENTICATE");
    len = sizeof(CK_BBOOL);
    b_tmp[0] = pvtkey_objects[piv_objects[obj].sub_id].always_auth;
    data = b_tmp;
    break;

  case CKA_MODIFIABLE:
    DBG("MODIFIABLE");
    len = sizeof(CK_BBOOL);
    b_tmp[0] = piv_objects[obj].modifiable;
    data = b_tmp;
    break;

  /* case CKA_SIGN_RECOVER: */
  /* case CKA_VENDOR_DEFINED: */
  /* case CKA_PRIVATE_EXPONENT: */
  /* case CKA_PRIME_1: */
  /* case CKA_PRIME_2: */
  /* case CKA_EXPONENT_1: */
  /* case CKA_EXPONENT_2: */
  /* case CKA_COEFFICIENT: */
  /* case CKA_PRIME: */
  /* case CKA_SUBPRIME: */
  /* case CKA_BASE: */
  /* case CKA_VALUE_BITS: */
  /* case CKA_VALUE_LEN: */
  /* case CKA_NEVER_EXTRACTABLE: */
  /* case CKA_ALWAYS_SENSITIVE:*/
  /* case CKA_START_DATE:  */
  /* case CKA_END_DATE: */
  default:
    DBG("UNKNOWN ATTRIBUTE %lx", template[0].type); // TODO: there are other parameters for public keys, plus there is more if the key is RSA
    template->ulValueLen = CK_UNAVAILABLE_INFORMATION;
    return CKR_ATTRIBUTE_TYPE_INVALID;
  }

  /* Just get the length */
  if (template->pValue == NULL_PTR) {
    template->ulValueLen = len;
    return CKR_OK;
  }

  /* Actually get the attribute */
  if (template->ulValueLen < len)
    return CKR_BUFFER_TOO_SMALL;

  template->ulValueLen = len;
  memcpy(template->pValue, data, len);

  return CKR_OK;
}

/* Get public key object attribute */
static CK_RV get_puoa(ykcs11_session_t *s, CK_OBJECT_HANDLE obj, CK_ATTRIBUTE_PTR template) {
  CK_BYTE_PTR data;
  CK_BYTE     b_tmp[1024];
  CK_ULONG    ul_tmp;
  CK_ULONG    len = 0;
  DBG("For public key object %lu, get ", obj);

  switch (template->type) {
  case CKA_CLASS:
    DBG("CLASS");
    len = sizeof(CK_ULONG);
    ul_tmp = CKO_PUBLIC_KEY;
    data = (CK_BYTE_PTR) &ul_tmp;
    break;

  case CKA_TOKEN:
    // Technically all these objects are token objects
    DBG("TOKEN");
    len = sizeof(CK_BBOOL);
    b_tmp[0] = piv_objects[obj].token;
    data = b_tmp;
    break;

  case CKA_PRIVATE:
    DBG("PRIVATE");
    len = sizeof(CK_BBOOL);
    b_tmp[0] = piv_objects[obj].private;
    data = b_tmp;
    break;

  case CKA_LABEL:
    DBG("LABEL");
    len = strlen(piv_objects[obj].label);
    data = (CK_BYTE_PTR) piv_objects[obj].label;
    break;

  case CKA_KEY_TYPE:
    DBG("KEY TYPE");
    len = sizeof(CK_ULONG);
    ul_tmp = do_get_key_type(s->pkeys[piv_objects[obj].sub_id]);
    if (ul_tmp == CKK_VENDOR_DEFINED) // This value is used as an error here
      return CKR_FUNCTION_FAILED;
    data = (CK_BYTE_PTR) &ul_tmp;
    break;

  case CKA_ID:
    DBG("ID");
    len = sizeof(CK_BYTE);
    b_tmp[0] = piv_objects[obj].sub_id;
    data = b_tmp;
    break;

  case CKA_SENSITIVE:
    DBG("SENSITIVE"); // Always false
    len = sizeof(CK_BBOOL);
    b_tmp[0] = 0;
    data = b_tmp;
    break;

  case CKA_LOCAL:
    DBG("LOCAL"); // Always false
    len = sizeof(CK_BBOOL);
    b_tmp[0] = 0;
    data = b_tmp;
    break;

  case CKA_ENCRYPT:
    DBG("ENCRYPT");
    len = sizeof(CK_BBOOL);
    b_tmp[0] = pubkey_objects[piv_objects[obj].sub_id].encrypt;
    data = b_tmp;
    break;

  case CKA_VERIFY: // TODO: what about verify recover ?
    DBG("VERIFY");
    len = sizeof(CK_BBOOL);
    b_tmp[0] = pubkey_objects[piv_objects[obj].sub_id].verify;
    data = b_tmp;
    break;

  case CKA_WRAP:
    DBG("WRAP");
    len = sizeof(CK_BBOOL);
    b_tmp[0] = pubkey_objects[piv_objects[obj].sub_id].wrap;
    data = b_tmp;
    break;

  case CKA_DERIVE:
    DBG("DERIVE");
    len = sizeof(CK_BBOOL);
    b_tmp[0] = pubkey_objects[piv_objects[obj].sub_id].derive;
    data = b_tmp;
    break;

  case CKA_EC_POINT:
    DBG("EC_POINT");
    len = sizeof(b_tmp);

    // Make sure that this is an EC key
    ul_tmp = do_get_key_type(s->pkeys[piv_objects[obj].sub_id]); // Getting the info from the pubk
    if (ul_tmp == CKK_VENDOR_DEFINED)
      return CKR_FUNCTION_FAILED;
    if (ul_tmp != CKK_ECDSA) {
      template->ulValueLen = CK_UNAVAILABLE_INFORMATION;
      return CKR_ATTRIBUTE_TYPE_INVALID;
    }

    if (do_get_public_key(s->pkeys[piv_objects[obj].sub_id], b_tmp, &len) != CKR_OK)
      return CKR_FUNCTION_FAILED;
    data = b_tmp;
    break;

  case CKA_EC_PARAMS:
    // Here we want the curve parameters (DER encoded OID)
    DBG("EC_PARAMS");
    len = sizeof(b_tmp);

    // Make sure that this is an EC key
    ul_tmp = do_get_key_type(s->pkeys[piv_objects[obj].sub_id]); // Getting the info from the pubk
    if (ul_tmp == CKK_VENDOR_DEFINED)
      return CKR_FUNCTION_FAILED;
    if (ul_tmp != CKK_ECDSA) {
      template->ulValueLen = CK_UNAVAILABLE_INFORMATION;
      return CKR_ATTRIBUTE_TYPE_INVALID;
    }

    if (do_get_curve_parameters(s->pkeys[piv_objects[obj].sub_id], b_tmp, &len) != CKR_OK)
      return CKR_FUNCTION_FAILED;

    data = b_tmp;
    break;

  case CKA_MODULUS:
    DBG("MODULUS");
    len = sizeof(b_tmp);

    // Make sure that this is an RSA key
    ul_tmp = do_get_key_type(s->pkeys[piv_objects[obj].sub_id]); // Getting the info from the pubk
    if (ul_tmp == CKK_VENDOR_DEFINED)
      return CKR_FUNCTION_FAILED;
    if (ul_tmp != CKK_RSA) {
      template->ulValueLen = CK_UNAVAILABLE_INFORMATION;
      return CKR_ATTRIBUTE_TYPE_INVALID;
    }

    if (do_get_modulus(s->pkeys[piv_objects[obj].sub_id], b_tmp, &len) != CKR_OK)
      return CKR_FUNCTION_FAILED;
    data = b_tmp;
    break;

  case CKA_MODULUS_BITS:
    DBG("MODULUS BITS");
    len = sizeof(CK_ULONG);

    // Make sure that this is an RSA key
    ul_tmp = do_get_key_type(s->pkeys[piv_objects[obj].sub_id]); // Getting the info from the pubk
    if (ul_tmp == CKK_VENDOR_DEFINED)
      return CKR_FUNCTION_FAILED;
    if (ul_tmp != CKK_RSA) {
      template->ulValueLen = CK_UNAVAILABLE_INFORMATION;
      return CKR_ATTRIBUTE_TYPE_INVALID;
    }

    ul_tmp = do_get_rsa_modulus_length(s->pkeys[piv_objects[obj].sub_id]); // Getting the info from the pubk
    if (ul_tmp == 0)
      return CKR_FUNCTION_FAILED;
    data = (CK_BYTE_PTR) &ul_tmp;
    break;

  case CKA_PUBLIC_EXPONENT:
    DBG("PUBLIC EXPONENT");
    len = sizeof(CK_ULONG);

    // Make sure that this is an RSA key
    ul_tmp = do_get_key_type(s->pkeys[piv_objects[obj].sub_id]); // Getting the info from the pubk
    if (ul_tmp == CKK_VENDOR_DEFINED)
      return CKR_FUNCTION_FAILED;
    if (ul_tmp != CKK_RSA) {
      template->ulValueLen = CK_UNAVAILABLE_INFORMATION;
      return CKR_ATTRIBUTE_TYPE_INVALID;
    }

    if (do_get_public_exponent(s->pkeys[piv_objects[obj].sub_id], b_tmp, &len) != CKR_OK)
      return CKR_FUNCTION_FAILED;
    data = b_tmp;
    break;

  case CKA_MODIFIABLE:
    DBG("MODIFIABLE");
    len = sizeof(CK_BBOOL);
    b_tmp[0] = piv_objects[obj].modifiable;
    data = b_tmp;
    break;

  /* case CKA_START_DATE: */
  /* case CKA_END_DATE: */
  default:
    DBG("UNKNOWN ATTRIBUTE %lx", template[0].type); // TODO: there are other parameters for public keys
    template->ulValueLen = CK_UNAVAILABLE_INFORMATION;
    return CKR_ATTRIBUTE_TYPE_INVALID;
  }

  /* Just get the length */
  if (template->pValue == NULL_PTR) {
    template->ulValueLen = len;
    return CKR_OK;
  }

  /* Actually get the attribute */
  if (template->ulValueLen < len)
    return CKR_BUFFER_TOO_SMALL;

  template->ulValueLen = len;
  memcpy(template->pValue, data, len);

  return CKR_OK;
}

CK_ULONG piv_2_ykpiv(piv_obj_id_t id) {
  switch(id) {
  case PIV_DATA_OBJ_CCC:
    return YKPIV_OBJ_CAPABILITY;

  case PIV_DATA_OBJ_CHUI:
    return YKPIV_OBJ_CHUID;

  case PIV_DATA_OBJ_CHF:
    return YKPIV_OBJ_FINGERPRINTS;

  case PIV_DATA_OBJ_SEC_OBJ:
    return YKPIV_OBJ_SECURITY;

  case PIV_DATA_OBJ_CHFI:
    return YKPIV_OBJ_FACIAL;

  case PIV_DATA_OBJ_PI:
    return YKPIV_OBJ_PRINTED;

  case PIV_DATA_OBJ_HISTORY:
    return YKPIV_OBJ_KEY_HISTORY;

  case PIV_DATA_OBJ_IRIS_IMAGE:
    return YKPIV_OBJ_IRIS;

  case PIV_DATA_OBJ_X509_PIV_AUTH:
  case PIV_CERT_OBJ_X509_PIV_AUTH:
    return YKPIV_OBJ_AUTHENTICATION;

  case PIV_DATA_OBJ_X509_CARD_AUTH:
  case PIV_CERT_OBJ_X509_CARD_AUTH:
    return YKPIV_OBJ_CARD_AUTH;

  case PIV_DATA_OBJ_X509_DS:
  case PIV_CERT_OBJ_X509_DS:
    return YKPIV_OBJ_SIGNATURE;

  case PIV_DATA_OBJ_X509_KM:
  case PIV_CERT_OBJ_X509_KM:
    return YKPIV_OBJ_KEY_MANAGEMENT;

  case PIV_DATA_OBJ_X509_RETIRED1:
  case PIV_CERT_OBJ_X509_RETIRED1:
    return YKPIV_OBJ_RETIRED1;

  case PIV_DATA_OBJ_X509_RETIRED2:
  case PIV_CERT_OBJ_X509_RETIRED2:
    return YKPIV_OBJ_RETIRED2;

  case PIV_DATA_OBJ_X509_RETIRED3:
  case PIV_CERT_OBJ_X509_RETIRED3:
    return YKPIV_OBJ_RETIRED3;

  case PIV_DATA_OBJ_X509_RETIRED4:
  case PIV_CERT_OBJ_X509_RETIRED4:
    return YKPIV_OBJ_RETIRED4;

  case PIV_DATA_OBJ_X509_RETIRED5:
  case PIV_CERT_OBJ_X509_RETIRED5:
    return YKPIV_OBJ_RETIRED5;

  case PIV_DATA_OBJ_X509_RETIRED6:
  case PIV_CERT_OBJ_X509_RETIRED6:
    return YKPIV_OBJ_RETIRED6;

  case PIV_DATA_OBJ_X509_RETIRED7:
  case PIV_CERT_OBJ_X509_RETIRED7:
    return YKPIV_OBJ_RETIRED7;

  case PIV_DATA_OBJ_X509_RETIRED8:
  case PIV_CERT_OBJ_X509_RETIRED8:
    return YKPIV_OBJ_RETIRED8;

  case PIV_DATA_OBJ_X509_RETIRED9:
  case PIV_CERT_OBJ_X509_RETIRED9:
    return YKPIV_OBJ_RETIRED9;

  case PIV_DATA_OBJ_X509_RETIRED10:
  case PIV_CERT_OBJ_X509_RETIRED10:
    return YKPIV_OBJ_RETIRED10;

  case PIV_DATA_OBJ_X509_RETIRED11:
  case PIV_CERT_OBJ_X509_RETIRED11:
    return YKPIV_OBJ_RETIRED11;

  case PIV_DATA_OBJ_X509_RETIRED12:
  case PIV_CERT_OBJ_X509_RETIRED12:
    return YKPIV_OBJ_RETIRED12;

  case PIV_DATA_OBJ_X509_RETIRED13:
  case PIV_CERT_OBJ_X509_RETIRED13:
    return YKPIV_OBJ_RETIRED13;

  case PIV_DATA_OBJ_X509_RETIRED14:
  case PIV_CERT_OBJ_X509_RETIRED14:
    return YKPIV_OBJ_RETIRED14;

  case PIV_DATA_OBJ_X509_RETIRED15:
  case PIV_CERT_OBJ_X509_RETIRED15:
    return YKPIV_OBJ_RETIRED15;

  case PIV_DATA_OBJ_X509_RETIRED16:
  case PIV_CERT_OBJ_X509_RETIRED16:
    return YKPIV_OBJ_RETIRED16;

  case PIV_DATA_OBJ_X509_RETIRED17:
  case PIV_CERT_OBJ_X509_RETIRED17:
    return YKPIV_OBJ_RETIRED17;

  case PIV_DATA_OBJ_X509_RETIRED18:
  case PIV_CERT_OBJ_X509_RETIRED18:
    return YKPIV_OBJ_RETIRED18;

  case PIV_DATA_OBJ_X509_RETIRED19:
  case PIV_CERT_OBJ_X509_RETIRED19:
    return YKPIV_OBJ_RETIRED19;

  case PIV_DATA_OBJ_X509_RETIRED20:
  case PIV_CERT_OBJ_X509_RETIRED20:
    return YKPIV_OBJ_RETIRED20;

  case PIV_DATA_OBJ_X509_ATTESTATION:
  case PIV_CERT_OBJ_X509_ATTESTATION:
    return YKPIV_OBJ_ATTESTATION;

  case PIV_PVTK_OBJ_PIV_AUTH:
    return YKPIV_KEY_AUTHENTICATION;

  case PIV_PVTK_OBJ_CARD_AUTH:
    return YKPIV_KEY_CARDAUTH;

  case PIV_PVTK_OBJ_DS:
    return YKPIV_KEY_SIGNATURE;

  case PIV_PVTK_OBJ_KM:
    return YKPIV_KEY_KEYMGM;

  case PIV_PVTK_OBJ_RETIRED1:
    return YKPIV_KEY_RETIRED1;

  case PIV_PVTK_OBJ_RETIRED2:
    return YKPIV_KEY_RETIRED2;

  case PIV_PVTK_OBJ_RETIRED3:
    return YKPIV_KEY_RETIRED3;

  case PIV_PVTK_OBJ_RETIRED4:
    return YKPIV_KEY_RETIRED4;

  case PIV_PVTK_OBJ_RETIRED5:
    return YKPIV_KEY_RETIRED5;

  case PIV_PVTK_OBJ_RETIRED6:
    return YKPIV_KEY_RETIRED6;

  case PIV_PVTK_OBJ_RETIRED7:
    return YKPIV_KEY_RETIRED7;

  case PIV_PVTK_OBJ_RETIRED8:
    return YKPIV_KEY_RETIRED8;

  case PIV_PVTK_OBJ_RETIRED9:
    return YKPIV_KEY_RETIRED9;

  case PIV_PVTK_OBJ_RETIRED10:
    return YKPIV_KEY_RETIRED10;

  case PIV_PVTK_OBJ_RETIRED11:
    return YKPIV_KEY_RETIRED11;

  case PIV_PVTK_OBJ_RETIRED12:
    return YKPIV_KEY_RETIRED12;

  case PIV_PVTK_OBJ_RETIRED13:
    return YKPIV_KEY_RETIRED13;

  case PIV_PVTK_OBJ_RETIRED14:
    return YKPIV_KEY_RETIRED14;

  case PIV_PVTK_OBJ_RETIRED15:
    return YKPIV_KEY_RETIRED15;

  case PIV_PVTK_OBJ_RETIRED16:
    return YKPIV_KEY_RETIRED16;

  case PIV_PVTK_OBJ_RETIRED17:
    return YKPIV_KEY_RETIRED17;

  case PIV_PVTK_OBJ_RETIRED18:
    return YKPIV_KEY_RETIRED18;

  case PIV_PVTK_OBJ_RETIRED19:
    return YKPIV_KEY_RETIRED19;

  case PIV_PVTK_OBJ_RETIRED20:
    return YKPIV_KEY_RETIRED20;

  case PIV_PVTK_OBJ_ATTESTATION:
    return YKPIV_KEY_ATTESTATION;

  default:
    return 0ul;
  }
}

static int compare_piv_obj_id(const void *a, const void *b) {
  return (*(piv_obj_id_t*)a - *(piv_obj_id_t*)b);
}

void sort_objects(ykcs11_session_t *s) {
  qsort(s->objects, s->n_objects, sizeof(piv_obj_id_t), compare_piv_obj_id);
}

CK_BBOOL is_present(ykcs11_session_t *s, piv_obj_id_t id) {
  return bsearch(&id, s->objects, s->n_objects, sizeof(piv_obj_id_t), compare_piv_obj_id) ? CK_TRUE : CK_FALSE;
}

CK_RV get_attribute(ykcs11_session_t *s, CK_OBJECT_HANDLE obj, CK_ATTRIBUTE_PTR template) {
  return piv_objects[obj].get_attribute(s, obj, template);
}

CK_BBOOL attribute_match(ykcs11_session_t *s, CK_OBJECT_HANDLE obj, CK_ATTRIBUTE_PTR attribute) {

  CK_ATTRIBUTE to_match;
  CK_BYTE_PTR data;

  // Get the size first
  to_match.type = attribute->type;
  to_match.pValue = NULL;
  to_match.ulValueLen = 0;

  if (get_attribute(s, obj, &to_match) != CKR_OK)
    return CK_FALSE;

  if (to_match.ulValueLen != attribute->ulValueLen)
    return CK_FALSE;

  // Allocate space for the attribute
  data = malloc(to_match.ulValueLen);
  if (data == NULL)
    return CK_FALSE;

  // Retrieve the attribute
  to_match.pValue = data;
  if (get_attribute(s, obj, &to_match) != CKR_OK) {
    free(data);
    data = NULL;
    return CK_FALSE;
  }

  // Compare the attributes
  if (memcmp(attribute->pValue, to_match.pValue, to_match.ulValueLen) != 0) {
    free(data);
    data = NULL;
    return CK_FALSE;
  }

  free(data);
  data = NULL;

  return CK_TRUE;
}

CK_BBOOL is_private_object(ykcs11_session_t *s, CK_OBJECT_HANDLE obj) {
  return (obj < PIV_DATA_OBJ_X509_PIV_AUTH || obj > PIV_PUBK_OBJ_ATTESTATION) ? CK_FALSE : piv_objects[obj].private;
}

CK_BYTE get_key_id(piv_obj_id_t obj) {
  return (obj < PIV_DATA_OBJ_X509_PIV_AUTH || obj > PIV_PUBK_OBJ_ATTESTATION) ? 0 : piv_objects[obj].sub_id;
}

piv_obj_id_t find_data_object(CK_BYTE sub_id) {
  for(piv_obj_id_t id = PIV_DATA_OBJ_X509_PIV_AUTH; id <= PIV_DATA_OBJ_X509_ATTESTATION; id++) {
    if(piv_objects[id].sub_id == sub_id)
      return id;
  }
  return -1;
}

piv_obj_id_t find_cert_object(CK_BYTE sub_id) {
  for(piv_obj_id_t id = PIV_CERT_OBJ_X509_PIV_AUTH; id <= PIV_CERT_OBJ_X509_ATTESTATION; id++) {
    if(piv_objects[id].sub_id == sub_id)
      return id;
  }
  return -1;
}

piv_obj_id_t find_pubk_object(CK_BYTE sub_id) {
  for(piv_obj_id_t id = PIV_PUBK_OBJ_PIV_AUTH; id <= PIV_PUBK_OBJ_ATTESTATION; id++) {
    if(piv_objects[id].sub_id == sub_id)
      return id;
  }
  return -1;
}

piv_obj_id_t find_pvtk_object(CK_BYTE sub_id) {
  for(piv_obj_id_t id = PIV_PVTK_OBJ_PIV_AUTH; id <= PIV_PVTK_OBJ_ATTESTATION; id++) {
    if(piv_objects[id].sub_id == sub_id)
      return id;
  }
  return -1;
}

CK_RV store_data(ykcs11_session_t *s, piv_obj_id_t obj, CK_BYTE_PTR data, CK_ULONG len) {
  s->data[piv_objects[obj].sub_id].data = realloc(s->data[piv_objects[obj].sub_id].data, len);
  if(s->data[piv_objects[obj].sub_id].data == NULL) {
    return CKR_HOST_MEMORY;
  }
  memcpy(s->data[piv_objects[obj].sub_id].data, data, len);
  s->data[piv_objects[obj].sub_id].len = len;
  return CKR_OK;
}

CK_RV store_cert(ykcs11_session_t *s, piv_obj_id_t cert_id, CK_BYTE_PTR data, CK_ULONG len) {

  CK_RV rv;

  // Store the certificate as an object
  rv = do_store_cert(data, len, &s->certs[piv_objects[cert_id].sub_id]);
  if (rv != CKR_OK)
    return rv;

  // Extract and store the public key as an object
  rv = do_store_pubk(s->certs[piv_objects[cert_id].sub_id], &s->pkeys[piv_objects[cert_id].sub_id]);
  if (rv != CKR_OK)
    return rv;

  return CKR_OK;
}

CK_RV delete_cert(ykcs11_session_t *s, piv_obj_id_t cert_id) {
  CK_RV rv;

  // Clear the object containing the certificate
  rv = do_delete_cert(&s->certs[piv_objects[cert_id].sub_id]);
  if (rv != CKR_OK)
    return rv;

  // Clear the object containing the public key
  rv = do_delete_pubk(&s->pkeys[piv_objects[cert_id].sub_id]);
  if (rv != CKR_OK)
    return rv;

  return CKR_OK;
}

CK_RV check_create_cert(CK_ATTRIBUTE_PTR templ, CK_ULONG n,
                        CK_BYTE_PTR id, CK_BYTE_PTR *value, CK_ULONG_PTR cert_len) {

  CK_ULONG    i;
  CK_BBOOL    has_id = CK_FALSE;
  CK_BBOOL    has_value = CK_FALSE;

  for (i = 0; i < n; i++) {
    switch (templ[i].type) {
    case CKA_CLASS:
      // Technically redundant check
      if (*((CK_ULONG_PTR)templ[i].pValue) != CKO_CERTIFICATE)
        return CKR_ATTRIBUTE_VALUE_INVALID;

      break;

    case CKA_ID:
      has_id = CK_TRUE;
      if (find_cert_object(*((CK_BYTE_PTR)templ[i].pValue)) == (piv_obj_id_t)-1)
        return CKR_ATTRIBUTE_VALUE_INVALID;

      *id = *((CK_BYTE_PTR)templ[i].pValue);
      break;

    case CKA_VALUE:
      has_value = CK_TRUE;
      *value = (CK_BYTE_PTR)templ[i].pValue;

      *cert_len = templ[i].ulValueLen;
      break;

    case CKA_TOKEN:
    case CKA_LABEL:
    case CKA_SUBJECT:
    case CKA_ISSUER:
    case CKA_CERTIFICATE_TYPE:
    case CKA_PRIVATE:
    case CKA_SERIAL_NUMBER:
      // Ignore other attributes
      break;

    default:
      DBG("Invalid %lx", templ[i].type);
      return CKR_ATTRIBUTE_TYPE_INVALID;
    }
  }

  if (has_id == CK_FALSE ||
      has_value == CK_FALSE)
    return CKR_TEMPLATE_INCOMPLETE;

  return CKR_OK;
}

CK_RV check_create_ec_key(CK_ATTRIBUTE_PTR templ, CK_ULONG n, CK_BYTE_PTR id,
                          CK_BYTE_PTR *value, CK_ULONG_PTR value_len) {

  CK_ULONG i;
  CK_BBOOL has_id = CK_FALSE;
  CK_BBOOL has_value = CK_FALSE;
  CK_BBOOL has_params = CK_FALSE;

  CK_BYTE_PTR ec_params = NULL;
  CK_ULONG    ec_params_len = 0;

  for (i = 0; i < n; i++) {
    switch (templ[i].type) {
    case CKA_CLASS:
      if (*((CK_ULONG_PTR)templ[i].pValue) != CKO_PRIVATE_KEY)
        return CKR_ATTRIBUTE_VALUE_INVALID;

      break;

    case CKA_KEY_TYPE:
      if (*((CK_ULONG_PTR)templ[i].pValue) != CKK_ECDSA)
        return CKR_ATTRIBUTE_VALUE_INVALID;

      break;

    case CKA_ID:
      has_id = CK_TRUE;
      if (find_pvtk_object(*((CK_BYTE_PTR)templ[i].pValue)) == (piv_obj_id_t)-1)
        return CKR_ATTRIBUTE_VALUE_INVALID;

      *id = *((CK_BYTE_PTR)templ[i].pValue);
      break;

    case CKA_VALUE:
      has_value = CK_TRUE;
      *value = (CK_BYTE_PTR)templ[i].pValue;
      *value_len = templ[i].ulValueLen;
      break;

    case CKA_EC_PARAMS:
      has_params = CK_TRUE;
      ec_params = (CK_BYTE_PTR)templ[i].pValue;
      ec_params_len = templ[i].ulValueLen;

      break;

    case CKA_TOKEN:
    case CKA_LABEL:
    case CKA_SUBJECT:
    case CKA_SENSITIVE:
    case CKA_DERIVE:
      // Ignore other attributes
      break;

    default:
      DBG("Invalid %lx", templ[i].type);
      return CKR_ATTRIBUTE_TYPE_INVALID;
    }
  }

  if (has_id == CK_FALSE ||
      has_value == CK_FALSE ||
      has_params == CK_FALSE)
    return CKR_TEMPLATE_INCOMPLETE;

  if (*value_len == 32 || *value_len == 31) {
    if (ec_params_len != 10 || memcmp(ec_params, PRIME256V1, ec_params_len) != 0)
      return CKR_ATTRIBUTE_VALUE_INVALID;
  }
  else if (*value_len == 48 || *value_len == 47) {
    if (ec_params_len != 7 || memcmp(ec_params, SECP384R1, ec_params_len) != 0)
      return CKR_ATTRIBUTE_VALUE_INVALID;
  }
  else {
    return CKR_ATTRIBUTE_VALUE_INVALID;
  }

  return CKR_OK;
}

CK_RV check_create_rsa_key(CK_ATTRIBUTE_PTR templ, CK_ULONG n, CK_BYTE_PTR id,
                           CK_BYTE_PTR *p, CK_ULONG_PTR p_len,
                           CK_BYTE_PTR *q, CK_ULONG_PTR q_len,
                           CK_BYTE_PTR *dp, CK_ULONG_PTR dp_len,
                           CK_BYTE_PTR *dq, CK_ULONG_PTR dq_len,
                           CK_BYTE_PTR *qinv, CK_ULONG_PTR qinv_len) {

  CK_ULONG i;
  CK_BBOOL has_id = CK_FALSE;
  CK_BBOOL has_e = CK_FALSE;
  CK_BBOOL has_p = CK_FALSE;
  CK_BBOOL has_q = CK_FALSE;
  CK_BBOOL has_dp = CK_FALSE;
  CK_BBOOL has_dq = CK_FALSE;
  CK_BBOOL has_qinv = CK_FALSE;

  for (i = 0; i < n; i++) {
    switch (templ[i].type) {
    case CKA_CLASS:
      if (*((CK_ULONG_PTR)templ[i].pValue) != CKO_PRIVATE_KEY)
        return CKR_ATTRIBUTE_VALUE_INVALID;

      break;

    case CKA_ID:
      has_id = CK_TRUE;
      if (find_pvtk_object(*((CK_BYTE_PTR)templ[i].pValue)) == (piv_obj_id_t)-1)
        return CKR_ATTRIBUTE_VALUE_INVALID;

      *id = *((CK_BYTE_PTR)templ[i].pValue);
      break;

    case CKA_KEY_TYPE:
      if (*((CK_ULONG_PTR)templ[i].pValue) != CKK_RSA)
        return CKR_ATTRIBUTE_VALUE_INVALID;

      break;

    case CKA_PUBLIC_EXPONENT:
      has_e = CK_TRUE;
      if (templ[i].ulValueLen != 3 || memcmp((CK_BYTE_PTR)templ[i].pValue, F4, 3) != 0)
        return CKR_ATTRIBUTE_VALUE_INVALID;
      break;

    case CKA_PRIME_1:
      has_p = CK_TRUE;
      *p = (CK_BYTE_PTR)templ[i].pValue;
      *p_len = templ[i].ulValueLen;

      break;

    case CKA_PRIME_2:
      has_q = CK_TRUE;
      *q = (CK_BYTE_PTR)templ[i].pValue;
      *q_len = templ[i].ulValueLen;

      break;

    case CKA_EXPONENT_1:
      has_dp = CK_TRUE;
      *dp = (CK_BYTE_PTR)templ[i].pValue;
      *dp_len = templ[i].ulValueLen;

      break;

    case CKA_EXPONENT_2:
      has_dq = CK_TRUE;
      *dq = (CK_BYTE_PTR)templ[i].pValue;
      *dq_len = templ[i].ulValueLen;

      break;

    case CKA_COEFFICIENT:
      has_qinv = CK_TRUE;
      *qinv = (CK_BYTE_PTR)templ[i].pValue;
      *qinv_len = templ[i].ulValueLen;

      break;

    case CKA_TOKEN:
    case CKA_LABEL:
    case CKA_SUBJECT:
    case CKA_SENSITIVE:
    case CKA_DERIVE:
      // Ignore other attributes
      break;

    default:
      DBG("Invalid %lx", templ[i].type);
      return CKR_ATTRIBUTE_TYPE_INVALID;
    }
  }

  if (has_id == CK_FALSE ||
      has_e == CK_FALSE ||
      has_p == CK_FALSE ||
      has_q == CK_FALSE ||
      has_dp == CK_FALSE ||
      has_dq == CK_FALSE ||
      has_qinv == CK_FALSE)
    return CKR_TEMPLATE_INCOMPLETE;

  if (*p_len != 64 && *p_len != 128)
    return CKR_ATTRIBUTE_VALUE_INVALID;


  if (*q_len != *p_len || *dp_len > *p_len ||
      *dq_len > *p_len || *qinv_len > *p_len)
    return CKR_ATTRIBUTE_VALUE_INVALID;

  return CKR_OK;
}

CK_RV check_delete_cert(CK_OBJECT_HANDLE hObject, CK_BYTE_PTR id) {

  if (hObject < PIV_CERT_OBJ_X509_PIV_AUTH || hObject > PIV_CERT_OBJ_X509_RETIRED20)
    return CKR_ARGUMENTS_BAD;

  *id = get_key_id(hObject);

  return CKR_OK;
}
