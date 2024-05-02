<?xml version="1.0"?>
 <xsl:stylesheet version="1.1" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:exsl="http://exslt.org/common"
        xmlns:math="http://exslt.org/math"
        extension-element-prefixes="exsl math">

 <!-- Output as a text file -->
 <xsl:output method="text"/>

 <!-- This comments removes forces us to explicitely -->
 <!-- put white space into the document -->
 <xsl:strip-space elements="*"/>

 <!-- Control all output through this template -->
 <xsl:template match="/">
 	<xsl:for-each select="//count/id[not(.=preceding::count/id)]">

		<xsl:sort data-type="number" select="parent::count/timestamp"/>
		<xsl:value-of select="."/>
		<xsl:text>&#10;</xsl:text>
	    <!--

		<xsl:text></xsl:text>
		<xsl:value-of select="."/>
		<xsl:if test="position()!=last()">
			<xsl:text>:</xsl:text>
		</xsl:if>
	    -->

	</xsl:for-each>

 </xsl:template>
 </xsl:stylesheet>
