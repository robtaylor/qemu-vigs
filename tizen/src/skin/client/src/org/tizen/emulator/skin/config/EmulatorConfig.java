/**
 * 
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Hyunjun Son <hj79.son@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

package org.tizen.emulator.skin.config;

import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Map;
import java.util.Properties;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.tizen.emulator.skin.dbi.EmulatorUI;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.IOUtil;
import org.tizen.emulator.skin.util.StringUtil;


/**
 * 
 *
 */
public class EmulatorConfig {
	
	private Logger logger = SkinLogger.getSkinLogger( EmulatorConfig.class ).getLogger();
	
	public interface ArgsConstants {
		public static final String UID = "uid";
		public static final String SERVER_PORT = "svr.port";
		public static final String RESOLUTION_WIDTH = "width";
		public static final String RESOLUTION_HEIGHT = "height";
		public static final String EMULATOR_NAME = "emulname";
		public static final String TEST_HEART_BEAT_IGNORE = "test.hb.ignore";
		public static final String VM_PATH = "vm.path";
		public static final String LOG_LEVEL = "log.level";
	}
	
	public interface PropertiesConstants {
		public static final String WINDOW_X = "window.x";
		public static final String WINDOW_Y = "window.y";
		public static final String WINDOW_DIRECTION = "window.rotate";
		public static final String WINDOW_SCALE = "window.scale";
	}
	
	private Map<String,String> args;
	private EmulatorUI dbiContents;
	private Properties properties;
	private String propertiesFilePath;
	
	public EmulatorConfig( Map<String,String> args, EmulatorUI dbiContents, Properties properties, String propertiesFilePath ) {
		this.args = args;
		this.dbiContents = dbiContents;
		this.properties = properties;
		this.propertiesFilePath = propertiesFilePath;
	}
	
	public void saveProperties() {
		
		FileOutputStream fos = null;
		
		try {
			
			fos = new FileOutputStream( propertiesFilePath );
			properties.store( fos, "Generated automatically by emulator." );
			
		} catch ( IOException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
		} finally {
			IOUtil.close( fos );
		}
		
	}

	public EmulatorUI getDbiContents() {
		return dbiContents;
	}

	public String getArg( String argKey ) {
		return args.get(argKey);
	}

	public String getArg( String argKey, String defaultValue ) {
		String arg = args.get( argKey );
		if ( StringUtil.isEmpty( arg ) ) {
			return defaultValue;
		} else {
			return arg;
		}
	}

	public String getProperty( String key ) {
		return properties.getProperty( key );
	}

	public String getProperty( String key, String defaultValue ) {
		String property = properties.getProperty( key );
		if( StringUtil.isEmpty( property ) ) {
			return defaultValue;
		}
		return property;
	}

	public int getPropertyInt( String key ) {
		return Integer.parseInt( properties.getProperty( key ) );
	}

	public int getPropertyInt( String key, int defaultValue ) {
		String property = properties.getProperty( key );
		if( StringUtil.isEmpty( property ) ) {
			return defaultValue;
		}
		return Integer.parseInt( property );
	}

	public short getPropertyShort( String key ) {
		return Short.parseShort( properties.getProperty( key ) );
	}

	public short getPropertyShort( String key, short defaultValue ) {
		String property = properties.getProperty( key );
		if( StringUtil.isEmpty( property ) ) {
			return defaultValue;
		}
		return Short.parseShort( property );
	}

	public void setProperty( String key, String value ) {
		properties.put( key, value );
	}
	
	public void setProperty( String key, int value ) {
		properties.put( key, Integer.toString( value ) );
	}

}
