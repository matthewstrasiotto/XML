#include "EventParse.h"
#include "DocParse.h"

#include "Utils.h"

#ifdef FROM_GNOME_XML_DIR
#include <gnome-xml/parserInternals.h>
#else
#include <libxml/parserInternals.h>
#endif


static USER_OBJECT_ createSAX2AttributesList(const xmlChar **attributes, int nb_attributes, int nb_defaulted);



/*
  This is an event driven parsing implementation for R & S
  using the libxml (http://xmlsoft.org) rather than Jim Clark's expat.
  It works much the same way, but has some advantages
   a) only one library need be installed for both document
     and event parsing
   b) the libxml tools can read data via ftp and http.

  Both expat and libxml provide the SAX interface and allow us to share
  a great deal of code between the two event parser implementations
  within this package.
 */

void RS_XML(startElementHandler)(void *ctx, const xmlChar *name, const xmlChar **atts);
void RS_XML(commentElementHandler)(void *ctx, const xmlChar *val);
void RS_XML(charactersHandler)(void *user_data, const xmlChar *ch, int len);
void RS_XML(endElementHandler)(void *ctx, const xmlChar *name);
void RS_XML(startDocumentHandler)(void *ctx);
void RS_XML(endDocumentHandler)(void *ctx);
void RS_XML(cdataBlockHandler)(void *ctx, const xmlChar *value, int len);
void RS_XML(piHandler)(void *ctx, const xmlChar *target, const xmlChar *data);
void RS_XML(entityDeclaration)(void *ctx, const xmlChar *name, int type, const xmlChar *publicId, 
                                const xmlChar *systemId, xmlChar *content);


int RS_XML(isStandAloneHandler)(void *ctx);
void RS_XML(warningHandler)(void *ctx, const char *msg, ...);
void RS_XML(errorHandler)(void *ctx, const char *format, ...); 
void RS_XML(fatalErrorHandler)(void *ctx, const char *msg, ...);
void RS_XML(structuredErrorHandler)(void *ctx, xmlErrorPtr err);


static void RS_XML(initXMLParserHandler)(xmlSAXHandlerPtr xmlParserHandler, int saxVersion);




#ifdef  NEED_CLOSE_CALLBACK
/* Is this actually needed? We can ensure that all errors
   are caught by R and so ensure that we close things.
*/
int
RS_XML_closeConnectionInput(void *context)
{
   int status;
   status = RS_XML_readConnectionInput(context, NULL, -1);

   return(1);
}
#endif

int
RS_XML_readConnectionInput(void *context, char *buffer, int len)
{
  SEXP e, tmp, arg;
  int n;
  int errorOccurred;
  char *str;
  int left = len-1, count;

#ifdef R_XML_DEBUG
  char *orig = buffer;
#endif

  xmlParserCtxtPtr ctx = (xmlParserCtxtPtr) context;

  if(len == -1)
     return(0);

  /* Setup the expression to call the user-supplied R function or call readLines(con, 1) 
     if they gave us a connection. */
  if(isFunction(ctx->_private)) {
     /* Invoke the user-provided function to get the next line. */
    PROTECT(e = allocVector(LANGSXP, 2));
    SETCAR(e, ctx->_private);
    PROTECT(arg = NEW_INTEGER(1));
    INTEGER_DATA(arg)[0] = len;
    SETCAR(CDR(e), arg);
    UNPROTECT(1);
  }

  n = count = 0;
  while(n == 0 && left > 0) {

   str = NULL;

     /* Update the argument to the user-defined function to say how much is left. */
   if(isFunction(ctx->_private))
     INTEGER_DATA(arg)[0] = left;

   tmp = R_tryEval(e, R_GlobalEnv, &errorOccurred);

   if(errorOccurred || !IS_CHARACTER(tmp)) {
     UNPROTECT(1);
     if ((ctx->sax != NULL) && (ctx->sax->error != NULL))  /* throw an XML error. */
           ctx->sax->error(ctx->userData, "Failed to call read on XML connection");
     return(-1);
   }

   if(GET_LENGTH(tmp)) {

      str = CHAR_DEREF(STRING_ELT(tmp, 0));
      n = strlen(str);


      if(n != 0) { /* Just add a new line and do it again. */

         if(n > left) {
            PROBLEM "string read from XML connection too long for buffer: truncating %s to %d characters", str, left
     	    WARN;
         }
         strncpy(buffer, str, left);
         left -= n;

         count += n ;
      }
   } else {
           /* Notice that we may have actually added something to the 
              buffer, specifically a sequence of empty lines \n, 
              and these will be discarded and not passed to the XML parser
              but these are extraneous anyway. Are they?
            */
      n = count = 0;
      break;
   }
  }

#ifdef R_XML_DEBUG
  fprintf(stderr, "size (len = %d, n=%d, count=%d)\nbuffer= '%s'\nRstring='%s'\n", len, n, count, buffer, str);fflush(stderr);
/*  fprintf(stderr, "size (n=%d, count=%d) %s '%s'\n", n, count, str, orig);fflush(stderr); */
#endif

  UNPROTECT(1);


  return(count);
/*  return(count == 0 ? -1 : count); */
}

xmlParserCtxtPtr
RS_XML_xmlCreateConnectionParserCtxt(USER_OBJECT_ con)
{
      xmlParserInputBufferPtr buf;
      xmlParserCtxtPtr ctx = NULL;

#ifdef LIBXML2
      ctx = xmlNewParserCtxt();
      ctx->_private = (USER_OBJECT_) con;
      buf = (xmlParserInputBufferPtr) R_chk_calloc(1, sizeof(xmlParserInputBuffer));
      buf->readcallback = RS_XML_readConnectionInput;
      buf->context = (void*) ctx;
      buf->buffer = xmlBufferCreate();
      buf->raw = NULL; /* buf->buffer; */

      ctx->input = xmlNewIOInputStream(ctx, buf, XML_CHAR_ENCODING_NONE);

      inputPush(ctx, ctx->input);
#endif
      return(ctx);
}

void
RS_XML(libXMLEventParse)(const char *fileName, RS_XMLParserData *parserData, RS_XML_ContentSourceType asText,
                          int saxVersion)
{
 xmlSAXHandlerPtr xmlParserHandler;
 xmlParserCtxtPtr ctx; /* = (xmlParserCtxtPtr) calloc(1, sizeof(xmlParserCtxtPtr)); XXX */
 int status;

  switch(asText) {
    case RS_XML_TEXT:
      ctx = xmlCreateDocParserCtxt(CHAR_TO_XMLCHAR(fileName));
      break;

    case RS_XML_FILENAME:
      ctx = xmlCreateFileParserCtxt(fileName);
      break;

    case RS_XML_CONNECTION:
      ctx = RS_XML_xmlCreateConnectionParserCtxt((USER_OBJECT_) fileName);

      break;
    default:
      ctx = NULL;
  }


  if(ctx == NULL) {
    PROBLEM "Can't parse %s", fileName
    ERROR;
  }


  xmlParserHandler = (xmlSAXHandlerPtr) S_alloc(sizeof(xmlSAXHandler), 1);/*XXX should this be S_alloc */
  /* Make certain this is initialized so that we don't have any references  to unwanted routines!  */
  memset(xmlParserHandler, '\0', sizeof(xmlSAXHandler));

  RS_XML(initXMLParserHandler)(xmlParserHandler, saxVersion);

  ctx->userData = parserData;
  ctx->sax = xmlParserHandler;

  status = xmlParseDocument(ctx);

  ctx->sax = NULL;
  xmlFreeParserCtxt(ctx);

/*  Free(xmlParserHandler); */
}





static void	
RS_XML(xmlSAX2StartElementNs)(void * userData, 
			      const xmlChar * localname, 
			      const xmlChar * prefix, 
			      const xmlChar * URI, 
			      int nb_namespaces, 
			      const xmlChar ** namespaces, 
			      int nb_attributes, 
			      int nb_defaulted, 
			      const xmlChar ** attributes)
{
  int i, n;
  USER_OBJECT_ tmp, names;
  USER_OBJECT_ opArgs;

  PROTECT(opArgs = NEW_LIST(4));
  SET_VECTOR_ELT(opArgs, 0, NEW_CHARACTER(1));
  SET_STRING_ELT(VECTOR_ELT(opArgs, 0), 0, COPY_TO_USER_STRING(XMLCHAR_TO_CHAR(localname))); 

      /* Now convert the attributes list. */
  SET_VECTOR_ELT(opArgs, 1, createSAX2AttributesList(attributes, nb_attributes, nb_defaulted));


  PROTECT(tmp = NEW_CHARACTER(1));
  if(URI) {
     SET_STRING_ELT(tmp, 0, COPY_TO_USER_STRING(XMLCHAR_TO_CHAR(URI))); 
     SET_NAMES(tmp, mkString((void*)prefix ? XMLCHAR_TO_CHAR(prefix) : "")); /*XXX not S-Plus compatible!*/
  }
  SET_VECTOR_ELT(opArgs, 2, tmp);
  UNPROTECT(1);

  n = nb_namespaces;
  PROTECT(tmp = NEW_CHARACTER(n));
  PROTECT(names = NEW_CHARACTER(n));
  for(i = 0, n = 0; n < nb_namespaces; n++, i+=2) {
      SET_STRING_ELT(tmp, n, COPY_TO_USER_STRING(XMLCHAR_TO_CHAR(namespaces[i+1])));
      if(namespaces[i])
         SET_STRING_ELT(names, n, COPY_TO_USER_STRING(XMLCHAR_TO_CHAR(namespaces[i])));
  }
  SET_NAMES(tmp, names);
  SET_VECTOR_ELT(opArgs, 3, tmp);
  UNPROTECT(2);


  RS_XML(callUserFunction)("startElement", XMLCHAR_TO_CHAR(localname), ((RS_XMLParserData*) userData), opArgs);

  UNPROTECT(1);
}

USER_OBJECT_
createSAX2AttributesList(const xmlChar **attributes, int nb_attributes, int nb_defaulted)
{
  int  i;
  const char **ptr;
  USER_OBJECT_ attr_names;
  USER_OBJECT_ attr_values;
  USER_OBJECT_ nsURI, nsNames;
 
  if(nb_attributes < 1)
    return(NULL_USER_OBJECT);

  PROTECT(attr_values = NEW_CHARACTER(nb_attributes));
  PROTECT(attr_names = NEW_CHARACTER(nb_attributes));

  PROTECT(nsURI = NEW_CHARACTER(nb_attributes));
  PROTECT(nsNames = NEW_CHARACTER(nb_attributes));

  ptr = (const char **) attributes; /*XXX */
  for(i=0; i < nb_attributes; i++, ptr+=5) {
      char *tmp;
      int len;

      len = (ptr[4] - ptr[3] + 1);
      tmp = malloc(sizeof(char) * len);
      if(!tmp) {
         PROBLEM "Cannot allocate space for attribute of length %d", (ptr[4] - ptr[3] + 2)
	 ERROR;
      }
      memcpy(tmp, ptr[3], ptr[4] - ptr[3]);
      tmp[len-1] = '\0'; /*XXX*/
      SET_STRING_ELT(attr_values, i,  COPY_TO_USER_STRING(tmp));
      free(tmp);

      SET_STRING_ELT(attr_names, i, COPY_TO_USER_STRING(ptr[0]));

      if(ptr[2]) {
         SET_STRING_ELT(nsURI, i,  COPY_TO_USER_STRING(ptr[2]));
         if(ptr[1])
            SET_STRING_ELT(nsNames, i,  COPY_TO_USER_STRING(ptr[1]));
      }
  }
  SET_NAMES(nsURI, nsNames);
  SET_NAMES(attr_values, attr_names);
  Rf_setAttrib(attr_values, Rf_install("namespaces"), nsURI);
  UNPROTECT(4);

  return(attr_values);
}



static void
RS_XML(xmlSAX2EndElementNs)(void * ctx, 
			    const xmlChar * localname, 
			    const xmlChar * prefix, 
			    const xmlChar * URI)
{
  USER_OBJECT_ args, tmp;

  PROTECT(args = NEW_LIST(2));
  SET_VECTOR_ELT(args, 0, mkString(XMLCHAR_TO_CHAR(localname)));

  PROTECT(tmp = mkString(( (XMLCHAR_TO_CHAR(URI)) ? XMLCHAR_TO_CHAR(URI) : ""))); 
  if(prefix)
     SET_NAMES(tmp, mkString(XMLCHAR_TO_CHAR(prefix)));
  SET_VECTOR_ELT(args, 1, tmp);

  RS_XML(callUserFunction)("endElement", NULL, (RS_XMLParserData *)ctx, args);

  UNPROTECT(2);
}



static void
RS_XML(xmlSAX2StartDocument)(void *userData)
{
}

void
RS_XML(initXMLParserHandler)(xmlSAXHandlerPtr xmlParserHandler, int saxVersion)
{

  if(saxVersion == 2) {
     xmlParserHandler->initialized = 0;
     xmlSAX2InitDefaultSAXHandler(xmlParserHandler, 0);
     xmlParserHandler->initialized = XML_SAX2_MAGIC;

     xmlParserHandler->startElementNs = RS_XML(xmlSAX2StartElementNs);
     xmlParserHandler->endElementNs = RS_XML(xmlSAX2EndElementNs);
     xmlParserHandler->startElement = NULL;
     xmlParserHandler->endElement = NULL;

     xmlParserHandler->serror = RS_XML(structuredErrorHandler);

  } else {
     xmlParserHandler->startElement = RS_XML(startElementHandler);
     xmlParserHandler->endElement = RS_XML(endElementHandler);
  }

  xmlParserHandler->comment = RS_XML(commentElementHandler);
  xmlParserHandler->entityDecl = RS_XML(entityDeclaration);
  xmlParserHandler->characters = RS_XML(charactersHandler);
  xmlParserHandler->processingInstruction = RS_XML(piHandler);
  xmlParserHandler->cdataBlock = RS_XML(cdataBlockHandler);

  xmlParserHandler->startDocument = RS_XML(startDocumentHandler);
  xmlParserHandler->endDocument = RS_XML(endDocumentHandler);

  xmlParserHandler->isStandalone = RS_XML(isStandAloneHandler);
  xmlParserHandler->fatalError = RS_XML(fatalErrorHandler);
  xmlParserHandler->warning = RS_XML(warningHandler);
  xmlParserHandler->error = RS_XML(errorHandler);

  /* external entity  */

  xmlParserHandler->internalSubset = NULL;
  xmlParserHandler->externalSubset = NULL;
  xmlParserHandler->hasInternalSubset = NULL;
  xmlParserHandler->hasExternalSubset = NULL;
  xmlParserHandler->resolveEntity = NULL;
  xmlParserHandler->getEntity = NULL;
  xmlParserHandler->getParameterEntity = NULL;
  xmlParserHandler->attributeDecl = NULL;
  xmlParserHandler->elementDecl = NULL;
  xmlParserHandler->notationDecl = NULL;
  xmlParserHandler->unparsedEntityDecl = NULL;
	       
  xmlParserHandler->setDocumentLocator = NULL;
  xmlParserHandler->reference = NULL;
  xmlParserHandler->ignorableWhitespace = NULL;


}

void
RS_XML(startElementHandler)(void *userData, const xmlChar *name, const xmlChar **atts)
{
  RS_XML(startElement)(userData, (const char *)name, (const char **)atts);
}

void
RS_XML(endElementHandler)(void *ctx, const xmlChar *name)
{
  RS_XML(endElement)(ctx, (const char *)name);
}

void
RS_XML(commentElementHandler)(void *ctx, const xmlChar *val)
{
  RS_XML(commentHandler)(ctx, (const XML_Char*)val);
}

void 
RS_XML(charactersHandler)(void *user_data, const xmlChar *ch, int len)
{
  RS_XML(textHandler)(user_data, (const XML_Char*)ch, len);
}

void
RS_XML(startDocumentHandler)(void *ctx)
{
  RS_XML(callUserFunction)("startDocument", NULL, ((RS_XMLParserData*) ctx), NULL_USER_OBJECT);
}

void
RS_XML(endDocumentHandler)(void *ctx)
{
  RS_XML(callUserFunction)("endDocument", NULL, ((RS_XMLParserData*) ctx), NULL_USER_OBJECT);
}

void
RS_XML(cdataBlockHandler)(void *ctx, const xmlChar *value, int len)
{
 USER_OBJECT_ opArgs;

 PROTECT(opArgs = NEW_LIST(1));
 SET_VECTOR_ELT(opArgs, 0, NEW_CHARACTER(1));
   SET_STRING_ELT(VECTOR_ELT(opArgs, 0), 0, COPY_TO_USER_STRING(XMLCHAR_TO_CHAR(value)));
  RS_XML(callUserFunction)("cdata", (const char *)NULL, (RS_XMLParserData*)ctx, opArgs);
  UNPROTECT(1);
}

void
RS_XML(piHandler)(void *ctx, const xmlChar *target, const xmlChar *data)
{
  RS_XML(processingInstructionHandler)(ctx, (const XML_Char*)target, (const XML_Char*)data);
}

void
RS_XML(entityDeclaration)(void *ctx,
                            const xmlChar *name, int type, const xmlChar *publicId,
			    const xmlChar *systemId, xmlChar *content)
{

}


int
RS_XML(isStandAloneHandler)(void *ctx)
{
  return(1);
}

void
RS_XML(fatalErrorHandler)(void *ctx, const char *format, ...)
{

  const char *msg = "error message unavailable";
  va_list args;
  va_start(args, format);

  if(strcmp(format, "%s") == 0)
    msg = va_arg(args, char *);

  va_end(args);


  PROBLEM "Fatal error in the XML event driven parser for %s: %s",
	  ((RS_XMLParserData*) ctx)->fileName, msg
  ERROR;

}

void
RS_XML(errorHandler)(void *ctx, const char *format, ...)
{
/*XXX Need to be smarter here about the msg coming from libxml containing formatting instructions
  e.g. %s and then picking up the ... */

  const char *msg = "error message unavailable";
  va_list args;
  va_start(args, format);

  if(strcmp(format, "%s") == 0)
    msg = va_arg(args, char *);

  va_end(args);

  PROBLEM "Error in the XML event driven parser for %s: %s",
    ((RS_XMLParserData*) ctx)->fileName, msg
  ERROR;

}

void 
RS_XML(structuredErrorHandler)(void *ctx, xmlErrorPtr err)
{
   if(err->level == XML_ERR_FATAL) {
      PROBLEM "Error in the XML event driven parser (line = %d, column = %d): %s",
         err->line, err->int2 , err->message
      ERROR;
   } else {
      PROBLEM "Error in the XML event driven parser (line = %d, column = %d): %s",
         err->line, err->int2 , err->message
      WARN;
   }
}

void
RS_XML(warningHandler)(void *ctx, const char *msg, ...)
{

 PROBLEM "XML event driven parser warning from %s.",
   ((RS_XMLParserData*) ctx)->fileName
 WARN;

}
