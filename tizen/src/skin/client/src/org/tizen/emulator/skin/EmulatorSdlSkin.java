/**
 * Emulator Skin Process
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
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

import java.lang.reflect.Field;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.PaletteData;
import org.tizen.emulator.skin.comm.ICommunicator.SendCommand;
import org.tizen.emulator.skin.comm.sock.data.DisplayStateData;
import org.tizen.emulator.skin.comm.sock.data.StartData;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.dbi.OptionType;
import org.tizen.emulator.skin.exception.ScreenShotException;
import org.tizen.emulator.skin.image.ImageRegistry.IconName;
import org.tizen.emulator.skin.info.SkinInformation;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.screenshot.SdlScreenShotWindow;
import org.tizen.emulator.skin.util.SkinUtil;
import org.tizen.emulator.skin.util.SwtUtil;

public class EmulatorSdlSkin extends EmulatorSkin {
	public static final int DISPLAY_COLOR_DEPTH = 32;

	private Logger logger = SkinLogger.getSkinLogger(
			EmulatorSdlSkin.class).getLogger();

	private PaletteData palette;

	/**
	 *  Constructor
	 */
	public EmulatorSdlSkin(EmulatorConfig config,
			SkinInformation skinInfo, boolean isOnTop) {
		super(config, skinInfo, SWT.EMBEDDED, isOnTop);

		/* BGRA */
		this.palette = new PaletteData(0x0000FF00, 0x00FF0000, 0xFF000000);
	}

	@Override
	public StartData initSkin() {
		initLayout();

		/* maru_sdl uses this handle ID */
		long id = getDisplayHandleId();

		/* generate a start data */
		int width = getEmulatorSkinState().getCurrentResolutionWidth();
		int height = getEmulatorSkinState().getCurrentResolutionHeight();
		int scale = getEmulatorSkinState().getCurrentScale();
		short rotation = getEmulatorSkinState().getCurrentRotationId();

		boolean isBlankGuide = true;
		OptionType option = config.getDbiContents().getOption();
		if (option != null) {
			isBlankGuide = (option.getBlankGuide() == null) ?
					true : option.getBlankGuide().isVisible();
		}

		StartData startData = new StartData(id,
				width, height, scale, rotation, isBlankGuide);
		logger.info("" + startData);

		return startData;
	}

	private long getDisplayHandleId() {
		long windowHandleId = 0;

		/* org.eclipse.swt.widgets.Widget */
		if (SwtUtil.isLinuxPlatform()) {
			try {
				Field field = lcdCanvas.getClass().getField("embeddedHandle");
				windowHandleId = field.getLong(lcdCanvas);
				logger.info("lcdCanvas.embeddedHandle:" + windowHandleId);
			} catch (IllegalArgumentException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				shutdown();
			} catch (IllegalAccessException e ) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				shutdown();
			} catch (SecurityException e ) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				shutdown();
			} catch (NoSuchFieldException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				shutdown();
			}
		} else if (SwtUtil.isWindowsPlatform()) {
			try {
				Field field = lcdCanvas.getClass().getField("handle");
				windowHandleId = field.getLong(lcdCanvas);
				logger.info("lcdCanvas.handle:" + windowHandleId);
			} catch (IllegalArgumentException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				shutdown();
			} catch (IllegalAccessException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				shutdown();
			} catch (SecurityException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				shutdown();
			} catch (NoSuchFieldException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				shutdown();
			}
		} else {
			/* not supported */
			logger.severe("Not Supported OS platform:" + SWT.getPlatform());
			shutdown();
		}

		return windowHandleId;
	}

	@Override
	public void updateDisplay() {
		logger.info("update display");

		DisplayStateData stateData = new DisplayStateData(
				currentState.getCurrentScale(),
				currentState.getCurrentRotationId());
		communicator.sendToQEMU(SendCommand.SEND_DISPLAY_STATE,
				stateData, false);
	}

	@Override
	public void displayOn() {
		super.displayOn();
	}

	@Override
	public void displayOff() {
		super.displayOff();
	}

	@Override
	protected void openScreenShotWindow() {
		if (screenShotDialog != null) {
			logger.info("screenshot window was already opened");
			return;
		}

		screenShotDialog = new SdlScreenShotWindow(this, config,
				palette, imageRegistry.getIcon(IconName.SCREENSHOT));

		try {
			screenShotDialog.open();
		} catch (ScreenShotException ex) {
			screenShotDialog = null;
			logger.log(Level.SEVERE, ex.getMessage(), ex);

			SkinUtil.openMessage(shell, null,
					"Fail to create a screen shot.", SWT.ICON_ERROR, config);
		} catch (Exception ex) {
			screenShotDialog = null;
			logger.log(Level.SEVERE, ex.getMessage(), ex);

			SkinUtil.openMessage(shell, null, "ScreenShot is not ready.\n" +
					"Please wait until the emulator is completely boot up.",
					SWT.ICON_WARNING, config);
		}
	}
}
