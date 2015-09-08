/*
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include "mobile/secure.h"
#include "mobile/ace_sha1.h"

#define SHA1_BLOCK_LEN		64	/* Input Message Block Length */
#define SHA1_DIGEST_LEN		20	/* Hash Code Length */

/*#define SOFT_SECURE		(1)*/

/* SHA1 Context  */
typedef struct {
	unsigned int auChain[SHA1_DIGEST_LEN / 4];	/* Chaining Variable */
	unsigned int auCount[2];	/* the number of input message bit */
	unsigned char abBuffer[SHA1_BLOCK_LEN];	/* Buffer for unfilled block */
} SHA1_ALG_INFO;

/* int sscl_memcmp(BYTE *pbSrc1, BYTE *pbSrc2, DWORD uByteLen); */
#define	macro_sscl_memcmp(BASE_FUNC_PTR,a,b,c) \
			(((int(*)(unsigned char *, unsigned char *, unsigned int))\
			(*((unsigned int *)(BASE_FUNC_PTR +  0))))\
			((a),(b),(c)))

/* void sscl_memcpy(BYTE *pbDst, BYTE *pbSrc, DWORD uByteLen); */
#define	macro_sscl_memcpy(BASE_FUNC_PTR,a,b,c) \
			(((void(*)(unsigned char *, unsigned char *, unsigned int))\
			(*((unsigned int *)(BASE_FUNC_PTR +  4))))\
			((a),(b),(c)))

//void sscl_memset(BYTE *pbDst, BYTE bValue, DWORD uByteLen); */
#define	macro_sscl_memset(BASE_FUNC_PTR,a,b,c) \
			(((void(*)(unsigned char *, unsigned char, unsigned int))\
			(*((unsigned int *)(BASE_FUNC_PTR +  8))))\
			((a),(b),(c)))

/* void sscl_memxor(BYTE *pbDst, BYTE *pbSrc1, BYTE *pbSrc2, DWORD uByteLen); */
#define	macro_sscl_memxor(BASE_FUNC_PTR,a,b,c,d) \
			(((void(*)(unsigned char *, unsigned char *, unsigned char *, unsigned int))\
			(*((unsigned int *)(BASE_FUNC_PTR + 12))))\
			((a),(b),(c),(d)))

/* unsigned int SEC_SHA1_Init(SHA1_ALG_INFO *psAlgInfo); */
#define	macro_SEC_SHA1_Init(BASE_FUNC_PTR,a) \
			(((unsigned int(*)(SHA1_ALG_INFO *))\
			(*((unsigned int *)(BASE_FUNC_PTR + 16))))\
			((a)))

/* unsigned SEC_SHA1_Update(SHA1_ALG_INFO *psAlgInfo, BYTE *pbMessage, DWORD uMsgLen); */
#define	macro_SEC_SHA1_Update(BASE_FUNC_PTR,a,b,c) \
			(((unsigned int(*)(SHA1_ALG_INFO *, unsigned char *, unsigned int))\
			(*((unsigned int *)(BASE_FUNC_PTR + 20))))\
			((a),(b),(c)))

/* unsigned SEC_SHA1_Final( SHA1_ALG_INFO *psAlgInfo, BYTE *pbDigest); */
#define	macro_SEC_SHA1_Final(BASE_FUNC_PTR,a,b) \
			(((unsigned int(*)(SHA1_ALG_INFO *, unsigned char *))\
			(*((unsigned int *)(BASE_FUNC_PTR + 24))))\
			((a),(b)))

/* RET_VAL SEC_SHA1_HMAC_SetInfo( SHA1_HMAC_ALG_INFO *psAlgInfo, BYTE *pbUserKey, DWORD uUKeyLen ); */
#define	macro_SEC_SHA1_HMAC_SetInfo(BASE_FUNC_PTR,a,b,c) \
			(((unsigned int(*)(SHA1_HMAC_ALG_INFO *, unsigned char *, unsigned int))\
			(*((unsigned int *)(BASE_FUNC_PTR + 28))))\
			((a),(b),(c)))

/* RET_VAL SEC_SHA1_HMAC_Init(SHA1_HMAC_ALG_INFO *psAlgInfo); */
#define	macro_SEC_SHA1_HMAC_Init(BASE_FUNC_PTR,a) \
			(((unsigned int(*)(SHA1_HMAC_ALG_INFO *))\
			(*((unsigned int *)(BASE_FUNC_PTR + 32))))\
			((a)))

/* RET_VAL SEC_SHA1_HMAC_Update(SHA1_HMAC_ALG_INFO *psAlgInfo, BYTE *pbMessage, DWORD uMsgLen); */
#define	macro_SEC_SHA1_HMAC_Update(BASE_FUNC_PTR,a,b,c) \
			(((unsigned int(*)(SHA1_HMAC_ALG_INFO *, unsigned char *, unsigned int))\
			(*((unsigned int *)(BASE_FUNC_PTR + 36))))\
			((a),(b),(c)))

/* RET_VAL SEC_SHA1_HMAC_Final( SHA1_HMAC_ALG_INFO *psAlgInfo, BYTE *pbHmacVal); */
#define	macro_SEC_SHA1_HMAC_Final(BASE_FUNC_PTR,a,b) \
			(((unsigned int(*)(SHA1_HMAC_ALG_INFO *, unsigned char *))\
			(*((unsigned int *)(BASE_FUNC_PTR + 40))))\
			((a),(b)))

/* int Verify_PSS_RSASignature (
 *	IN	unsigned char	*rawRSAPublicKey,
 *	IN	int		rawRSAPublicKeyLen,
 *	IN	unsigned char	*hashCode,
 *	IN	int		hashCodeLen,
 *	IN	unsigned char	*signature,
 *	IN	int		signatureLen);
 */
#define	macro_Verify_PSS_RSASignature(BASE_FUNC_PTR,a,b,c,d,e,f) \
			(((int(*)(unsigned char *, int, unsigned char *, int, unsigned char*, int))\
			(*((unsigned int *)(BASE_FUNC_PTR + 44))))\
			((a),(b),(c),(d),(e),(f)))

/* Verify integrity of BL2(or OS) Image. */
static int Check_IntegrityOfImage(IN SecureBoot_CTX * sbContext,
				  IN unsigned char *BL2,
				  IN int BL2Len,
				  IN unsigned char *BL2_SignedData,
				  IN int BL2_SignedDataLen)
{
	unsigned int rv;
	unsigned char hashCode[SHA1_DIGEST_LEN];
	int hashCodeLen = SHA1_DIGEST_LEN;
	unsigned int SBoot_BaseFunc_ptr;
	SHA1_ALG_INFO algInfo;
	RawRSAPublicKey tempPubKey;
	SBoot_BaseFunc_ptr = (unsigned int)sbContext->func_ptr_BaseAddr;

	/* 0. if stage2 pubkey is 0x00, do NOT check integrity. */
	macro_sscl_memset(SBoot_BaseFunc_ptr,
			  (unsigned char *)&tempPubKey,
			  0x00, sizeof(RawRSAPublicKey));

	rv = macro_sscl_memcmp(SBoot_BaseFunc_ptr,
			       (unsigned char *)&(sbContext->stage2PubKey),
			       (unsigned char *)&tempPubKey,
			       sizeof(RawRSAPublicKey));

	if (rv == 0)
		return SB_OFF;
	/* 1. Make HashCode */
	/* 1-1. SHA1 Init */
	macro_sscl_memset(SBoot_BaseFunc_ptr,
			  (unsigned char *)&algInfo,
			  0x00, sizeof(SHA1_ALG_INFO));

#if defined(SOFT_SECURE)
	macro_SEC_SHA1_Init(SBoot_BaseFunc_ptr, &algInfo);

	/* 1-3. SHA1 Update. */
	macro_SEC_SHA1_Update(SBoot_BaseFunc_ptr, &algInfo, BL2, BL2Len);

	/* 1-4. SHA1 Final. */
	macro_SEC_SHA1_Final(SBoot_BaseFunc_ptr, &algInfo, hashCode);
#else
	SHA1_digest(hashCode, BL2, BL2Len);
#endif
	/* 2. BL2's signature verification */
	rv = macro_Verify_PSS_RSASignature(SBoot_BaseFunc_ptr,
					   (unsigned char *)&(sbContext->
							      stage2PubKey),
					   sizeof(RawRSAPublicKey), hashCode,
					   hashCodeLen, BL2_SignedData,
					   BL2_SignedDataLen);
	if (rv != SB_OK)
		return rv;

	return SB_OK;
}

void check_secure_key()
{
#ifdef CONFIG_SECURE_BOOTING
	volatile unsigned long *pub_key;
	int rv = 0;

	int rw = 0;
	int i = 0;
	int secure_booting;

	if (rw == 0) {
		pub_key = (volatile unsigned long *)SECURE_KEY_ADDRESS;
		secure_booting = 0;

		for (i = 0; i < 33; i++) {
			if (*(pub_key + i) != 0x0) {
				secure_booting = 1;
				break;
			}
		}

		printf("Secure Boot: %s\n", secure_booting ? "On" : "Off");

		if (secure_booting == 1) {
			rv = Check_IntegrityOfImage((SecureBoot_CTX *)
						    SECURE_KEY_ADDRESS,
						    (unsigned char *)
						    CONFIG_KERNEL_BASE_ADDRESS,
						    (CONFIG_KERNEL_SIZE -
						     SECURE_KEY_SIZE),
						    (unsigned char
						     *)
						    (CONFIG_KERNEL_BASE_ADDRESS
						     + (CONFIG_KERNEL_SIZE -
							SECURE_KEY_SIZE)),
						    SECURE_KEY_SIZE);

			if (rv != 0) {
				printf(" Integrity check failed : 0x%08x\n");
				while (1) ;
			}
		} else {
			printf("Invalid public key\n");
			while (1) ;
		}
	}
#endif
}
