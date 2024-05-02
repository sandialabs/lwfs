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


 <xsl:variable name="total">
    <xsl:value-of select="count(/sddf/count[id=$id])"/>
 </xsl:variable>

 <xsl:variable name="min">
    <xsl:value-of select="math:min(/sddf/count[id=$id]/timestamp)"/>
 </xsl:variable>

 <xsl:variable name="max">
    <xsl:value-of select="math:max(/sddf/count[id=$id]/timestamp)"/>
 </xsl:variable>



 <!-- Do nothing unless a template matches exactly -->
 <xsl:template match="/">
    <xsl:call-template name="header"/>
    <xsl:call-template name="countdata"/>
 </xsl:template>


 <!-- HEADER -->
 <xsl:template name="header" match="/sddf/stream">

    <xsl:text>% Counter data from LWFS Experiment (count events with id=</xsl:text>
    <xsl:value-of select="$id"/>
    <xsl:text>)&#10;</xsl:text>
    <xsl:text>%&#10;</xsl:text>

    <xsl:for-each select="/sddf/stream/attr">
	<xsl:text>% </xsl:text>
	<xsl:value-of select="@key"/>
	<xsl:text>=</xsl:text>
	<xsl:value-of select="@value"/>
	<xsl:text>&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>% &#10;</xsl:text>

    <xsl:text>% </xsl:text>
    <xsl:value-of select="$total"/>
    <xsl:text> Events&#10;</xsl:text>

    <xsl:text>% Range (sec) [</xsl:text>
    <xsl:value-of select="$min"/>
    <xsl:text>, </xsl:text>
    <xsl:value-of select="$max"/>
    <xsl:text>] &#10;</xsl:text>


    <xsl:text>% Columns:&#10;</xsl:text>
    <xsl:text>%   1: Event Identifier &#10;</xsl:text>
    <xsl:text>%   2: Time (sec)&#10;</xsl:text>
    <xsl:text>%   3: Count &#10;</xsl:text>
    <xsl:text>% &#10;</xsl:text>
    
    <xsl:value-of select="stream"/>
 </xsl:template>

 <!-- Output information from the description fields -->
 <xsl:template name="descr" match="/sddf/descr">
    <xsl:for-each select="/sddf/descr/attr">
	<xsl:value-of select="@key"/>
	<xsl:text>&#10;</xsl:text>
    </xsl:for-each>
 </xsl:template>

 <!-- COUNTER DATA -->
 <xsl:template name="countdata" match="/sddf">

    <xsl:for-each select="/sddf/count[id=$id]">
	<xsl:value-of select="id"/>
	<xsl:text> </xsl:text>
	<xsl:value-of select="timestamp"/>
	<xsl:text> </xsl:text>
	<xsl:value-of select="count"/>
	<xsl:text>&#10;</xsl:text>
    </xsl:for-each>

 </xsl:template>

 </xsl:stylesheet>
