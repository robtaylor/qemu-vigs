//
// This file was generated by the JavaTM Architecture for XML Binding(JAXB) Reference Implementation, vJAXB 2.1.10 in JDK 6 
// See <a href="http://java.sun.com/xml/jaxb">http://java.sun.com/xml/jaxb</a> 
// Any modifications to this file will be lost upon recompilation of the source schema. 
// Generated on: 2012.03.30 at 05:41:59 오후 KST 
//


package org.tizen.emulator.skin.dbi;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlType;


/**
 * <p>Java class for colorsType complex type.
 * 
 * <p>The following schema fragment specifies the expected content contained within this class.
 * 
 * <pre>
 * &lt;complexType name="colorsType">
 *   &lt;complexContent>
 *     &lt;restriction base="{http://www.w3.org/2001/XMLSchema}anyType">
 *       &lt;all>
 *         &lt;element name="hoverColor" type="{http://www.tizen.org/emulator/dbi}rgbType" minOccurs="0"/>
 *       &lt;/all>
 *     &lt;/restriction>
 *   &lt;/complexContent>
 * &lt;/complexType>
 * </pre>
 * 
 * 
 */
@XmlAccessorType(XmlAccessType.FIELD)
@XmlType(name = "colorsType", propOrder = {

})
public class ColorsType {

    protected RgbType hoverColor;

    /**
     * Gets the value of the hoverColor property.
     * 
     * @return
     *     possible object is
     *     {@link RgbType }
     *     
     */
    public RgbType getHoverColor() {
        return hoverColor;
    }

    /**
     * Sets the value of the hoverColor property.
     * 
     * @param value
     *     allowed object is
     *     {@link RgbType }
     *     
     */
    public void setHoverColor(RgbType value) {
        this.hoverColor = value;
    }

}
