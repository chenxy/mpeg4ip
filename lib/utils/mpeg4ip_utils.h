/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is MPEG4IP.
 *
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 *
 * Contributor(s):
 *              Dave Mackie             dmackie@cisco.com
 */


#ifndef __MPEG4IP_UTILS__
#define __MPEG4IP_UTILS__ 1

#ifdef __cplusplus
extern "C" {
#endif

  char *convert_url(const char *to_convert);

  void message(int loglevel, const char *lib, const char *fmt, ...)
#ifndef _WIN32
       __attribute__((format(__printf__, 3, 4)))
#endif
       ;
  void library_message(int loglevel,
		       const char *lib,
		       const char *fmt,
		       va_list ap);

char *get_host_ip_address(void);

#ifdef __cplusplus
}
#endif

#endif