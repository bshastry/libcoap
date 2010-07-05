/* pdu.c -- CoAP message structure
 *
 * (c) 2010 Olaf Bergmann <bergmann@tzi.org>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "pdu.h"


coap_pdu_t *
coap_new_pdu() {
  coap_pdu_t *pdu = malloc( sizeof(coap_pdu_t) + COAP_MAX_PDU_SIZE );
  if (!pdu) {
    perror("new_pdu: malloc");
    return NULL;
  }
  
  /* initialize PDU */
  memset(pdu, 0, sizeof(coap_pdu_t) + COAP_MAX_PDU_SIZE );
  pdu->hdr = (coap_hdr_t *) ( (unsigned char *)pdu + sizeof(coap_pdu_t) );
  pdu->hdr->version = COAP_DEFAULT_VERSION;

  /* data points after the header; when options are added, the data 
     pointer is moved to the back */
  pdu->length = sizeof(coap_hdr_t);
  pdu->data = (unsigned char *)pdu->hdr + pdu->length;

  return pdu;
}

void 
coap_delete_pdu(coap_pdu_t *pdu) {
  free( pdu );
}

#define options_start(p) ((coap_opt_t *) ( (unsigned char *)p->hdr + sizeof ( coap_hdr_t ) ))

int 
coap_add_option(coap_pdu_t *pdu, unsigned char type, unsigned int len, const unsigned char *data) {
  unsigned char cnt;
  coap_opt_t *opt;
  unsigned char opt_code = 0;

  if (!pdu) 
    return -1;

  /* get last option from pdu to calculate the delta */
  
  opt = options_start( pdu );
  for ( cnt = pdu->hdr->optcnt; cnt; --cnt ) {
    opt_code += COAP_OPT_DELTA(*opt);
    opt = (coap_opt_t *)( (unsigned char *)opt + COAP_OPT_SIZE(*opt) ); 
  }

  if ( type < opt_code ) {
#ifndef NDEBUG
    fprintf(stderr, "options not added in correct order\n");
#endif
    return -1;
  }

  /* create new option after last existing option */
  pdu->hdr->optcnt += 1;
  COAP_OPT_SETDELTA( *opt, type - opt_code );
  
  COAP_OPT_SETLENGTH( *opt, len );
  memcpy(COAP_OPT_VALUE(*opt), data, len);
  pdu->data = (unsigned char *)COAP_OPT_VALUE(*opt) + len ;

  pdu->length = pdu->data - (unsigned char *)pdu->hdr;
  return len;
}

int 
coap_add_data(coap_pdu_t *pdu, unsigned int len, const unsigned char *data) {
  if ( !pdu )
    return 0;
  
  if ( pdu->length + len > COAP_MAX_PDU_SIZE ) {
#ifndef NDEBUG
    fprintf(stderr, "coap_add_data: cannot add: data too large for PDU\n");
#endif
    return 0;
  }

  memcpy( (unsigned char *)pdu->hdr + pdu->length, data, len );
  pdu->length += len;
  return 1;
}