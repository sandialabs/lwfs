<?xml version="1.0"?>
 <xsl:stylesheet version="1.1" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:exsl="http://exslt.org/common"
        xmlns:math="http://exslt.org/math"
        extension-element-prefixes="exsl math">

 <!-- Generates a histogram of the number of events that occur within -->
 <!-- a given amount of time (e.g., 1 sec).  The input parameters help -->
 <!-- select the type of entries.  -->

 <!-- Output as a text file -->
 <xsl:output method="text" />

 <!-- This comments removes forces us to explicitely -->
 <!-- put white space into the document -->
 <xsl:strip-space elements="*"/>

 <xsl:param name="id" select="2"/>
 <xsl:param name="scaling" select="1"/>

 <!-- HEADER -->


 <!-- Print the header information -->
 <!--
 <xsl:template name="header" match="/sddf/desc">
    <xsl:value-of select="."/>
 -->
    <!--
    <xsl:text>% Histogram data from LWFS Experiment&#10;</xsl:text>
    <xsl:text>% Experiment date=</xsl:text>
    <xsl:value-of select="."/>
    <xsl:value-of select="attr[@key='run date']/@value"/>
    <xsl:text>% Experiment time=&#10;</xsl:text>
    -->
 <!--
 </xsl:template>
 -->

 <!-- Do nothing unless a template matches exactly -->
 <xsl:template match="/">
    <xsl:call-template name="header"/>
    <xsl:call-template name="descr"/>
 </xsl:template>


 <!-- Output header information -->
 <xsl:template name="header" match="/sddf/stream">

    <xsl:text>% Histogram data from LWFS Experiment (Events with id=</xsl:text>
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

    <xsl:text>% Columns:&#10;</xsl:text>
    <xsl:text>%   1: Event Identifier &#10;</xsl:text>
    <xsl:text>%   2: Bucket Identifier&#10;</xsl:text>
    <xsl:text>%   3: Scaling Factor&#10;</xsl:text>
    <xsl:text>%   4: Count&#10;</xsl:text>
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

 </xsl:stylesheet>
