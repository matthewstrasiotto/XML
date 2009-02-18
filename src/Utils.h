/*
 * See Copyright for the license status of this software.
 */

#ifndef UTILS_H
#define UTILS_H

#include "RS_XML.h"
#include "RSCommon.h"

#define XMLCHAR_TO_CHAR(val)  ((char *) val)
#define CHAR_TO_XMLCHAR(val)  ((xmlChar *) val)

int isBlank(const char *str);
char *trim(char *str);

#include <libxml/parser.h>

  /* name of the R class identifying a function that wants the xmlParserCtxt as the first argument. */
#define XML_PARSE_CONTEXT_FUNCTION "XMLParserContextFunction"

#define XML_PARSER_CONTEXT_TYPE_NAME "XMLParserContext"

USER_OBJECT_ RS_XML(invokeFunction)(USER_OBJECT_ fun, USER_OBJECT_ opArgs, USER_OBJECT_ state, xmlParserCtxtPtr ctx);
USER_OBJECT_ RS_XML(findFunction)(const char *opName, USER_OBJECT_ functions);

void RS_XML(SetNames)(int n, const char *cnames[], USER_OBJECT_ ans);
int RS_XML(SetClassName)(const char *name, USER_OBJECT_ target);

SEXP R_makeRefObject(void *ref, const char *className);

#ifndef SET_CLASS_NAME
#define SET_CLASS_NAME(localClassName, target)  RS_XML(SetClassName)((localClassName), (target))
#endif

#ifdef LIBXML2
#ifdef FROM_GNOME_XML_DIR
#include <gnome-xml/hash.h>
#else
#include <libxml/hash.h>
#endif

int xmlHashSize(xmlHashTablePtr table);
#endif


void RSXML_setErrorHandlers(void);


USER_OBJECT_ RS_XML(RecursiveApply)(USER_OBJECT_ top, USER_OBJECT_ func, USER_OBJECT_ klasses);
USER_OBJECT_ RS_XML(HtmlParseTree)(USER_OBJECT_ fileName, USER_OBJECT_ converterFunctions, 
				   USER_OBJECT_ skipBlankLines, USER_OBJECT_ replaceEntities,
				   USER_OBJECT_ asText, USER_OBJECT_ trim, USER_OBJECT_ isURL);

USER_OBJECT_ RS_XML(getDTD)(USER_OBJECT_ dtdFileName, USER_OBJECT_ externalId, 
			    USER_OBJECT_ asText, USER_OBJECT_ isURL, USER_OBJECT_ errorFun);
USER_OBJECT_ RS_XML(libxmlVersion)();


USER_OBJECT_ 
RS_XML(Parse)(USER_OBJECT_ fileName, USER_OBJECT_ handlers, USER_OBJECT_ endElementHandlers, USER_OBJECT_ addContext, 
               USER_OBJECT_ ignoreBlanks,  USER_OBJECT_ useTagName, USER_OBJECT_ asText,
                 USER_OBJECT_ trim, USER_OBJECT_ useExpat, USER_OBJECT_ stateObject,
                  USER_OBJECT_ replaceEntities, USER_OBJECT_ validate, USER_OBJECT_ saxVersion,
 	            USER_OBJECT_ branches, USER_OBJECT_ useDotNames, USER_OBJECT_ errorFun);
/*
USER_OBJECT_ RS_XML(Parse)(USER_OBJECT_ fileName, USER_OBJECT_ handlers, USER_OBJECT_ addContext, 
			   USER_OBJECT_ ignoreBlanks,  USER_OBJECT_ useTagName, USER_OBJECT_ asText,
			   USER_OBJECT_ trim, USER_OBJECT_ useExpat, USER_OBJECT_ stateObject,
			   USER_OBJECT_ replaceEntities, USER_OBJECT_ validate);
*/

USER_OBJECT_
RS_XML(ParseTree)(USER_OBJECT_ fileName, USER_OBJECT_ converterFunctions, 
		  USER_OBJECT_ skipBlankLines, USER_OBJECT_ replaceEntities,
		  USER_OBJECT_ asText, USER_OBJECT_ trim, USER_OBJECT_ validate,
		  USER_OBJECT_ getDTD, USER_OBJECT_ isURL,
		  USER_OBJECT_ addNamespaceAttributes, USER_OBJECT_ useInternalNodes,
		  USER_OBJECT_ s_useHTML, USER_OBJECT_ isSchema, USER_OBJECT_ fullNamespaceInfo, USER_OBJECT_ r_encoding,
                  USER_OBJECT_ useDotNames, USER_OBJECT_ xinclude, USER_OBJECT_ errorFun);

USER_OBJECT_ R_newXMLDtd(USER_OBJECT_ sdoc, USER_OBJECT_ sname, USER_OBJECT_ sexternalID, USER_OBJECT_ ssysID);
USER_OBJECT_ R_newXMLDoc(USER_OBJECT_ dtd, USER_OBJECT_ namespaces);

USER_OBJECT_ R_newXMLNode(USER_OBJECT_ name, USER_OBJECT_ attrs, USER_OBJECT_ nameSpace, USER_OBJECT_ sdoc, USER_OBJECT_ namespaceDefinitions);
USER_OBJECT_ R_newXMLTextNode(USER_OBJECT_ value, USER_OBJECT_ sdoc);
USER_OBJECT_ R_xmlNewComment(USER_OBJECT_ str, USER_OBJECT_ sdoc);
USER_OBJECT_ R_newXMLCDataNode(USER_OBJECT_ sdoc, USER_OBJECT_ value);
USER_OBJECT_ R_newXMLPINode(USER_OBJECT_ sdoc, USER_OBJECT_ name, USER_OBJECT_ content);
USER_OBJECT_ R_xmlNewNs(USER_OBJECT_ sdoc, USER_OBJECT_ shref, USER_OBJECT_ sprefix);
USER_OBJECT_ R_xmlSetNs(USER_OBJECT_ s_node, USER_OBJECT_ s_ns, USER_OBJECT_ append);
USER_OBJECT_ R_insertXMLNode(USER_OBJECT_ node, USER_OBJECT_ parent, USER_OBJECT_ r_at, USER_OBJECT_ shallow) ;
USER_OBJECT_ R_saveXMLDOM(USER_OBJECT_ sdoc, USER_OBJECT_ sfileName, USER_OBJECT_ compression, USER_OBJECT_ sindent, 
			  USER_OBJECT_ prefix, USER_OBJECT_ r_encoding);


USER_OBJECT_ RS_XML_xmlNodeNumChildren(USER_OBJECT_ snode);

USER_OBJECT_ R_createXMLNodeRef(xmlNodePtr node);
USER_OBJECT_ R_createXMLDocRef(xmlDocPtr doc);

USER_OBJECT_ R_xmlCatalogResolve(SEXP r_id, SEXP type, USER_OBJECT_ debug);

USER_OBJECT_ RS_XML_setDoc(USER_OBJECT_ snode, USER_OBJECT_ sdoc);
USER_OBJECT_ RS_XML_unsetDoc(USER_OBJECT_ snode, USER_OBJECT_ unlink, USER_OBJECT_ r_parent);

USER_OBJECT_ RS_XML_printXMLNode(USER_OBJECT_ node, USER_OBJECT_ level, USER_OBJECT_ format, USER_OBJECT_ indent, USER_OBJECT_ r_encoding);

USER_OBJECT_ RS_XML_removeChildren(USER_OBJECT_ s_node, USER_OBJECT_ kids, USER_OBJECT_ freeNode);

USER_OBJECT_ RS_XML_clone(USER_OBJECT_ obj, USER_OBJECT_ recursive, USER_OBJECT_ addFinalizer);




Rboolean R_isInstanceOf(USER_OBJECT_ obj, const char *klass);
USER_OBJECT_ RS_XML_addNodeAttributes(USER_OBJECT_ s_node, USER_OBJECT_ attrs);
USER_OBJECT_ RS_XML_removeNodeAttributes(USER_OBJECT_ s_node, USER_OBJECT_ attrs, USER_OBJECT_ asNamespace);
USER_OBJECT_ RS_XML_getNsList(USER_OBJECT_ s_node, USER_OBJECT_ asRef);

USER_OBJECT_ RS_XML_setNodeName(USER_OBJECT_ s_node, USER_OBJECT_ s_name);
USER_OBJECT_ R_xmlNsAsCharacter(USER_OBJECT_ s_ns);


USER_OBJECT_ R_createXMLNsRef(xmlNsPtr ns);
USER_OBJECT_ RS_XML_getNextSibling(USER_OBJECT_ node, USER_OBJECT_ s_prev);

USER_OBJECT_ R_getXMLNodeDocument(USER_OBJECT_ s_node);
USER_OBJECT_ RS_XML_createDocFromNode(USER_OBJECT_ s_node);

SEXP R_removeInternalNode(SEXP r_node, SEXP r_free);

USER_OBJECT_ RS_XML_replaceXMLNode(USER_OBJECT_ r_old, USER_OBJECT_ r_new);


USER_OBJECT_ RS_XML_xmlAddSiblingAt(USER_OBJECT_ r_to, USER_OBJECT_ r_node, USER_OBJECT_ r_before);

SEXP RS_XML_clearCatalog();
SEXP RS_XML_loadCatalog(SEXP catalogs);
SEXP RS_XML_catalogAdd(SEXP orig, SEXP replace, SEXP type);
SEXP RS_XML_catalogDump(SEXP fileName);

void R_xmlFreeDoc(SEXP ref);

USER_OBJECT_ RS_XML_setDocumentName(USER_OBJECT_ sdoc, USER_OBJECT_ sname);

USER_OBJECT_ RS_XML_setKeepBlanksDefault(USER_OBJECT_ val);



SEXP stop(const char *className, const char *msg, ...);
SEXP RSXML_structuredStop(SEXP errorFun, xmlErrorPtr err);

void R_xmlStructuredErrorHandler(void *data, xmlErrorPtr err);


SEXP R_getDocEncoding(SEXP r_doc);

extern int R_numXMLDocs, R_numXMLDocsFreed;

SEXP CreateCharSexpWithEncoding(const xmlChar *encoding, const xmlChar *str);

#define DECL_ENCODING_FROM_NODE(node)   const xmlChar *encoding = node->doc ? node->doc->encoding : NULL;
#define DECL_ENCODING_FROM_DOC(doc)   const xmlChar *encoding = doc->encoding;
#define DECL_ENCODING_FROM_EVENT_PARSER(parserData)   const xmlChar *encoding = parserData->ctx->encoding;

#ifdef R_USE_XML_ENCODING
#undef COPY_TO_USER_STRING
#warning "Redefining COPY_TO_USER_STRING to use encoding from XML parser"
#define COPY_TO_USER_STRING(x)  CreateCharSexpWithEncoding(encoding, CHAR_TO_XMLCHAR (x))
#endif


#include <R_ext/Utils.h>
#define R_CHECK_INTERRUPTS R_CheckUserInterrupt();

#endif

