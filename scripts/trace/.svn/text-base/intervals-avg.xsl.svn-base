<?xml version="1.0"?>
 <xsl:stylesheet version="1.1" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:exsl="http://exslt.org/common"
        xmlns:math="http://exslt.org/math"
        extension-element-prefixes="exsl math">

 <!-- Creates a data file that outputs the type, timestamp, and count -->
 <!-- of each count event with an id matching the "id" parameter -->

 <!-- Output as a text file -->
 <xsl:output method="text" />

 <!-- This comments removes forces us to explicitely -->
 <!-- put white space into the document -->
 <xsl:strip-space elements="*"/>

 <xsl:param name="id" select="2"/>
 <xsl:param name="output_details" select="0"/>

 <xsl:variable name="intervals" select="//interval[id=$id]"/>

 <xsl:template match="/">
 <xsl:value-of select="sum($intervals/duration) div count($intervals)"/>
 <xsl:text>&#10;</xsl:text>
 </xsl:template>

 </xsl:stylesheet>
