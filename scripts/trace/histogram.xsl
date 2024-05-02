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
 <xsl:param name="binsize" select="1"/>
 <xsl:param name="output_details" select="0"/>


 <xsl:variable name="total">
    <xsl:value-of select="count(/sddf/*[id=$id])"/>
 </xsl:variable>

 <xsl:variable name="min">
    <xsl:value-of select="math:min(/sddf/*[id=$id]/timestamp)"/>
 </xsl:variable>

 <xsl:variable name="max">
    <xsl:value-of select="math:max(/sddf/*[id=$id]/timestamp)"/>
 </xsl:variable>



 <!-- Do nothing unless a template matches exactly -->
 <xsl:template match="/">
    <xsl:call-template name="header"/>
    <xsl:call-template name="histogram"/>
 </xsl:template>


 <!-- HEADER -->
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

    <xsl:text>% </xsl:text>
    <xsl:value-of select="$total"/>
    <xsl:text> Events&#10;</xsl:text>

    <xsl:text>% Range (sec) [</xsl:text>
    <xsl:value-of select="$min"/>
    <xsl:text>, </xsl:text>
    <xsl:value-of select="$max"/>
    <xsl:text>] &#10;</xsl:text>

    <xsl:text>% Binsize (sec) =  </xsl:text>
    <xsl:value-of select="$binsize"/>
    <xsl:text> &#10;</xsl:text>
    <xsl:text>% &#10;</xsl:text>



    <xsl:text>% Columns:&#10;</xsl:text>
    <xsl:text>%   1: Event Identifier &#10;</xsl:text>
    <xsl:text>%   2: Min Time (sec)&#10;</xsl:text>
    <xsl:text>%   2: Max Time (sec)&#10;</xsl:text>
    <xsl:text>%   3: Count (events between min and max)&#10;</xsl:text>
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

 <!-- HISTOGRAM DATA -->
 <xsl:template name="histogram" match="/sddf">

    <xsl:param name="current" select="floor($max div $binsize) * $binsize"/>

    <xsl:if test="($current) > $min">
		<xsl:call-template name="histogram">
		    <xsl:with-param name="current" select="$current - $binsize"/>
		</xsl:call-template>
    </xsl:if>

    <xsl:variable name="localmin">
	<xsl:if test="$current &lt; 0">
	    <xsl:value-of select="0"/>
	</xsl:if>

	<xsl:if test="$current &gt;= 0">
	    <xsl:value-of select="$current"/>
	</xsl:if>

    </xsl:variable>

    <xsl:variable name="localmax">
	<xsl:value-of select="$localmin + $binsize"/>
    </xsl:variable>


    <xsl:value-of select="$id"/>
    <!--
    <xsl:text> [</xsl:text>
    -->
    <xsl:text> </xsl:text>
    <xsl:value-of select="$localmin"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="$localmax"/>
    <xsl:text> </xsl:text>
    <!--
    <xsl:text>] </xsl:text>
    -->


    <xsl:value-of select="count(sddf/*[id=$id][timestamp &lt; $localmax and timestamp &gt;= $localmin]/timestamp)"/>

    <xsl:text> </xsl:text>
    <xsl:text>&#10;</xsl:text>

    <xsl:if test="$output_details &gt; 0">
	<xsl:for-each select="sddf/*[id=$id][timestamp &lt; $localmax and timestamp &gt;= $localmin]">
	    <xsl:value-of select="id"/>
	    <xsl:text>    "</xsl:text>
	    <xsl:value-of select="data"/>
	    <xsl:text>"    </xsl:text>
	    <xsl:value-of select="timestamp"/>
	    <xsl:text>&#10;</xsl:text>
	</xsl:for-each>
	<xsl:text>&#10;</xsl:text>
    </xsl:if>

 </xsl:template>

 </xsl:stylesheet>
