<?xml version="1.0"?>
 <xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
 <xsl:output method="text" />

 <xsl:template match="/">
 	<xsl:value-of select="count(//lwfs-interval/node[not(.=preceding::lwfs-interval/node)])"/>
 </xsl:template>
 </xsl:stylesheet>
