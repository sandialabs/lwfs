<?xml version="1.0"?>
 <xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
 <xsl:output method="text" />

 <xsl:template match="/">
 	<xsl:for-each select="//lwfs-interval/node[not(.=preceding::lwfs-interval/node)]">
		<xsl:sort select="parent::lwfs-interval/start"/>

		<xsl:text></xsl:text>
		<xsl:value-of select="."/>
		<xsl:if test="position()!=last()">
			<xsl:text>:</xsl:text>
		</xsl:if>

	</xsl:for-each>

 </xsl:template>
 </xsl:stylesheet>
