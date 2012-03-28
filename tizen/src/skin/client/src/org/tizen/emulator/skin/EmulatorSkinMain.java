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

package org.tizen.emulator.skin;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.Socket;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.widgets.Display;
import org.tizen.emulator.skin.EmulatorSkin.SkinReopenPolicy;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.config.EmulatorConfig.ConfigPropertiesConstants;
import org.tizen.emulator.skin.config.EmulatorConfig.SkinPropertiesConstants;
import org.tizen.emulator.skin.dbi.EmulatorUI;
import org.tizen.emulator.skin.exception.JaxbException;
import org.tizen.emulator.skin.image.ImageRegistry;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.log.SkinLogger.SkinLogLevel;
import org.tizen.emulator.skin.util.IOUtil;
import org.tizen.emulator.skin.util.JaxbUtil;
import org.tizen.emulator.skin.util.StringUtil;

/**
 * 
 *
 */
public class EmulatorSkinMain {

	public static final String SKIN_PROPERTIES_FILE_NAME = ".skin.properties";
	public static final String CONFIG_PROPERTIES_FILE_NAME = ".skinconfig.properties";
	public static final String DBI_FILE_NAME = "default.dbi";

	private static Logger logger;
	
	/**
	 * @param args
	 */
	public static void main( String[] args ) {
		
		try {

			Map<String, String> argsMap = parsArgs( args );

			String vmPath = (String) argsMap.get( ArgsConstants.VM_PATH );
			
			String skinPropFilePath = vmPath + File.separator + SKIN_PROPERTIES_FILE_NAME;
			Properties skinProperties = loadProperties( skinPropFilePath, true );
			if ( null == skinProperties ) {
				System.err.println( "[SkinLog]Fail to load skin properties file." );
				System.exit( -1 );
			}

			String configPropFilePath = vmPath + File.separator + CONFIG_PROPERTIES_FILE_NAME;
			Properties configProperties = loadProperties( configPropFilePath, false );

			// able to use log file after loading properties
			initLog( argsMap, configProperties );

			int lcdWidth = Integer.parseInt( argsMap.get( ArgsConstants.RESOLUTION_WIDTH ) );
			int lcdHeight = Integer.parseInt( argsMap.get( ArgsConstants.RESOLUTION_HEIGHT ) );
			String argSkinPath = (String) argsMap.get( ArgsConstants.SKIN_PATH );

			EmulatorUI dbiContents = loadDbi( argSkinPath, lcdWidth, lcdHeight );
			if ( null == dbiContents ) {
				logger.severe( "Fail to load dbi file." );
				System.exit( -1 );
			}

			EmulatorConfig config = new EmulatorConfig( argsMap, dbiContents, skinProperties, skinPropFilePath,
					configProperties );

			ImageRegistry.getInstance().initialize( config );
			
			String onTopVal = config.getSkinProperty( SkinPropertiesConstants.WINDOW_ONTOP, Boolean.FALSE.toString() );
			boolean isOnTop = Boolean.parseBoolean( onTopVal );

			EmulatorSkin skin = new EmulatorSkin( config, isOnTop );
			int windowHandleId = skin.compose();

			int uid = Integer.parseInt( config.getArg( ArgsConstants.UID ) );
			SocketCommunicator communicator = new SocketCommunicator( config, uid, windowHandleId, skin );

			skin.setCommunicator( communicator );

			Socket commSocket = communicator.getSocket();

			if ( null != commSocket ) {

				Runtime.getRuntime().addShutdownHook( new EmulatorShutdownhook( communicator ) );

				Thread communicatorThread = new Thread( communicator );
				communicatorThread.start();
				
				SkinReopenPolicy reopenPolicy = skin.open();
				
				while( true ) {

					if( null != reopenPolicy ) {
						
						if( reopenPolicy.isReopen() ) {
							
							EmulatorSkin reopenSkin = reopenPolicy.getReopenSkin();
							logger.info( "Reopen skin dialog." );
							reopenPolicy = reopenSkin.open();
							
						}else {
							break;
						}
						
					}else {
						break;
					}

				}
				
			} else {
				logger.severe( "CommSocket is null." );
			}

		} catch ( Throwable e ) {

			if ( null != logger ) {
				logger.log( Level.SEVERE, e.getMessage(), e );
			} else {
				e.printStackTrace();
			}

		} finally {
			ImageRegistry.getInstance().dispose();
			Display.getDefault().close();
			SkinLogger.end();
		}

	}

	private static void initLog( Map<String, String> argsMap, Properties properties ) {

		String argLogLevel = argsMap.get( ArgsConstants.LOG_LEVEL );
		String configPropertyLogLevel = null;
		
		if( null != properties ) {
			configPropertyLogLevel = (String) properties.get( ConfigPropertiesConstants.LOG_LEVEL );
		}

		// default log level is debug.
		
		String logLevel = "";
		
		if( !StringUtil.isEmpty( argLogLevel ) ) {
			logLevel = argLogLevel;
		}else if( !StringUtil.isEmpty( configPropertyLogLevel ) ) {
			logLevel = configPropertyLogLevel;
		}else {
			logLevel = SkinLogLevel.DEBUG.value();
		}
		
		SkinLogLevel skinLogLevel = SkinLogLevel.DEBUG;
		
		SkinLogLevel[] values = SkinLogLevel.values();
		
		for ( SkinLogLevel level : values ) {
			if ( level.value().equalsIgnoreCase( logLevel ) ) {
				skinLogLevel = level;
				break;
			}
		}
		
		String vmPath = argsMap.get( ArgsConstants.VM_PATH );
		
		SkinLogger.init( skinLogLevel, vmPath );
		
		logger = SkinLogger.getSkinLogger( EmulatorSkinMain.class ).getLogger();
		
	}

	private static Map<String, String> parsArgs( String[] args ) {

		Map<String, String> map = new HashMap<String, String>();

		for ( int i = 0; i < args.length; i++ ) {
			String arg = args[i];
			System.out.println( "[SkinLog]arg[" + i + "] " + arg );
			String[] split = arg.split( "=" );

			if ( 1 < split.length ) {

				String argKey = split[0].trim();
				String argValue = split[1].trim();
				System.out.println( "[SkinLog]argKey:" + argKey + "  argValue:" + argValue );
				map.put( argKey, argValue );

			} else {
				System.out.println( "[SkinLog]only one argv:" + arg );
			}
		}

		System.out.println( "[SkinLog]========================================" );
		System.out.println( "[SkinLog]args:" + map );
		System.out.println( "[SkinLog]========================================" );

		return map;

	}

	private static EmulatorUI loadDbi( String argSkinPath, int lcdWidth, int lcdHeight ) {

		String skinPath = ImageRegistry.getSkinPath( argSkinPath, lcdWidth, lcdHeight ) + File.separator + DBI_FILE_NAME;

		FileInputStream fis = null;
		EmulatorUI emulatorUI = null;

		try {
			
			fis = new FileInputStream( skinPath );
			logger.info( "============ dbi contents ============" );
			byte[] bytes = IOUtil.getBytes( fis );
			logger.info( new String( bytes, "UTF-8" ) );
			logger.info( "=======================================" );
			
			emulatorUI = JaxbUtil.unmarshal( bytes, EmulatorUI.class );
			
		} catch ( IOException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
		} catch ( JaxbException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
		} finally {
			IOUtil.close( fis );
		}

		return emulatorUI;

	}

	private static Properties loadProperties( String filePath, boolean create ) {

		FileInputStream fis = null;
		Properties properties = null;

		try {

			File file = new File( filePath );
			
			if( create ) {
				
				if ( !file.exists() ) {
					if ( !file.createNewFile() ) {
						System.err.println( "[SkinLog]Fail to create new " + filePath + " property file." );
						return null;
					}
				}
				
				fis = new FileInputStream( filePath );
				properties = new Properties();
				properties.load( fis );
				
			}else {
				
				if ( file.exists() ) {

					fis = new FileInputStream( filePath );
					properties = new Properties();
					properties.load( fis );
				}
				
			}

			System.out.println( "[SkinLog]load properties file : " + filePath );

		} catch ( IOException e ) {
			System.err.println( "[SkinLog]Fail to load skin properties file." );
			e.printStackTrace();
		} finally {
			IOUtil.close( fis );
		}

		return properties;

	}
	
}
