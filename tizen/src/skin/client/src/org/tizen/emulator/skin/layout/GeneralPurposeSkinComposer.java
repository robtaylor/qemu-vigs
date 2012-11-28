/**
 * 
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
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

package org.tizen.emulator.skin.layout;

import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.EmulatorSkinState;
import org.tizen.emulator.skin.comm.ICommunicator.RotationInfo;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.config.EmulatorConfig.SkinPropertiesConstants;
import org.tizen.emulator.skin.image.ImageRegistry;
import org.tizen.emulator.skin.image.ImageRegistry.IconName;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.SkinRotation;
import org.tizen.emulator.skin.util.SkinUtil;
import org.tizen.emulator.skin.util.SwtUtil;

public class GeneralPurposeSkinComposer implements ISkinComposer {
	private Logger logger = SkinLogger.getSkinLogger(
			GeneralPurposeSkinComposer.class).getLogger();

	private EmulatorConfig config;
	private Shell shell;
	private Canvas lcdCanvas;
	private EmulatorSkinState currentState;

	private ImageRegistry imageRegistry;
	private SkinPatches frame;
	private SocketCommunicator communicator;

	private PaintListener shellPaintListener;
	private MouseMoveListener shellMouseMoveListener;
	private MouseListener shellMouseListener;

	private boolean isGrabbedShell;
	private Point grabPosition;

	public GeneralPurposeSkinComposer(EmulatorConfig config, Shell shell,
			EmulatorSkinState currentState, ImageRegistry imageRegistry,
			SocketCommunicator communicator) {
		this.config = config;
		this.shell = shell;
		this.currentState = currentState;
		this.imageRegistry = imageRegistry;
		this.communicator = communicator; //TODO: delete
		this.isGrabbedShell= false;
		this.grabPosition = new Point(0, 0);

		this.frame = new SkinPatches("images/emul-window/");
	}

	@Override
	public Canvas compose() {
		lcdCanvas = new Canvas(shell, SWT.EMBEDDED); //TODO:

		int x = config.getSkinPropertyInt(SkinPropertiesConstants.WINDOW_X,
				EmulatorConfig.DEFAULT_WINDOW_X);
		int y = config.getSkinPropertyInt(SkinPropertiesConstants.WINDOW_Y,
				EmulatorConfig.DEFAULT_WINDOW_Y);

		currentState.setCurrentResolutionWidth(
				config.getArgInt(ArgsConstants.RESOLUTION_WIDTH));
		currentState.setCurrentResolutionHeight(
				config.getArgInt(ArgsConstants.RESOLUTION_HEIGHT));

		int scale = SkinUtil.getValidScale(config);
		short rotationId = EmulatorConfig.DEFAULT_WINDOW_ROTATION;

		composeInternal(lcdCanvas, x, y, scale, rotationId);
		logger.info("resolution : " + currentState.getCurrentResolution() +
				", scale : " + scale);

		return lcdCanvas;
	}

	@Override
	public void composeInternal(Canvas lcdCanvas,
			int x, int y, int scale, short rotationId) {

		//shell.setBackground(shell.getDisplay().getSystemColor(SWT.COLOR_BLACK));
		shell.setLocation(x, y);

		String emulatorName = SkinUtil.makeEmulatorName(config);
		shell.setText(emulatorName);

		lcdCanvas.setBackground(shell.getDisplay().getSystemColor(SWT.COLOR_BLACK));

		if (SwtUtil.isWindowsPlatform()) {
			shell.setImage(imageRegistry.getIcon(IconName.EMULATOR_TITLE_ICO));
		} else {
			shell.setImage(imageRegistry.getIcon(IconName.EMULATOR_TITLE));
		}

		arrangeSkin(scale, rotationId);
	}

	@Override
	public void arrangeSkin(int scale, short rotationId) {
		currentState.setCurrentScale(scale);
		currentState.setCurrentRotationId(rotationId);
		currentState.setCurrentAngle(SkinRotation.getAngle(rotationId));

		/* arrange the lcd */
		Rectangle lcdBounds = adjustLcdGeometry(lcdCanvas,
				currentState.getCurrentResolutionWidth(),
				currentState.getCurrentResolutionHeight(), scale, rotationId);

		if (lcdBounds == null) {
			logger.severe("Failed to lcd information for phone shape skin.");
			SkinUtil.openMessage(shell, null,
					"Failed to read lcd information for phone shape skin.\n" +
					"Check the contents of skin dbi file.",
					SWT.ICON_ERROR, config);
			System.exit(-1);
		}
		logger.info("lcd bounds : " + lcdBounds);

		lcdCanvas.setBounds(lcdBounds);

		/* arrange the skin image */
		Image tempImage = null;

		if (currentState.getCurrentImage() != null) {
			tempImage = currentState.getCurrentImage();
		}

		currentState.setCurrentImage(
				frame.getPatchedImage(lcdBounds.width, lcdBounds.height));

		if (tempImage != null) {
			tempImage.dispose();
		}

		/* custom window shape */
		SkinUtil.trimShell(shell, currentState.getCurrentImage());

		/* set window size */
		if (currentState.getCurrentImage() != null) {
			ImageData imageData = currentState.getCurrentImage().getImageData();
			shell.setMinimumSize(imageData.width, imageData.height);
			shell.setSize(imageData.width, imageData.height);
		}

		shell.pack();
		shell.redraw();
	}

	@Override
	public Rectangle adjustLcdGeometry(
			Canvas lcdCanvas, int resolutionW, int resolutionH,
			int scale, short rotationId) {

		Rectangle lcdBounds = new Rectangle(
				frame.getPatchWidth(), frame.getPatchHeight(), 0, 0);

		float convertedScale = SkinUtil.convertScale(scale);
		RotationInfo rotation = RotationInfo.getValue(rotationId);

		/* resoultion, that is lcd size in general skin mode */
		if (RotationInfo.LANDSCAPE == rotation ||
				RotationInfo.REVERSE_LANDSCAPE == rotation) {
			lcdBounds.width = (int)(resolutionH * convertedScale);
			lcdBounds.height = (int)(resolutionW * convertedScale);
		} else {
			lcdBounds.width = (int)(resolutionW * convertedScale);
			lcdBounds.height = (int)(resolutionH * convertedScale);
		}

		return lcdBounds;
	}

	public void addGeneralPurposeListener(final Shell shell) {
		shellPaintListener = new PaintListener() {
			@Override
			public void paintControl(final PaintEvent e) {
				/* general shell does not support native transparency,
				 * so draw image with GC. */
				if (currentState.getCurrentImage() != null) {
					e.gc.drawImage(currentState.getCurrentImage(), 0, 0);
				}

			}
		};

		shell.addPaintListener(shellPaintListener);

		shellMouseMoveListener = new MouseMoveListener() {
			@Override
			public void mouseMove(MouseEvent e) {
				if (isGrabbedShell == true && e.button == 0/* left button */ &&
						currentState.getCurrentPressedHWKey() == null) {
					/* move a window */
					Point previousLocation = shell.getLocation();
					int x = previousLocation.x + (e.x - grabPosition.x);
					int y = previousLocation.y + (e.y - grabPosition.y);

					shell.setLocation(x, y);
					return;
				}
			}
		};

		shell.addMouseMoveListener(shellMouseMoveListener);

		shellMouseListener = new MouseListener() {
			@Override
			public void mouseUp(MouseEvent e) {
				if (e.button == 1) { /* left button */
					logger.info("mouseUp in Skin");

					isGrabbedShell = false;
					grabPosition.x = grabPosition.y = 0;
				}
			}

			@Override
			public void mouseDown(MouseEvent e) {
				if (1 == e.button) { /* left button */
					logger.info("mouseDown in Skin");

					isGrabbedShell = true;
					grabPosition.x = e.x;
					grabPosition.y = e.y;
				}
			}

			@Override
			public void mouseDoubleClick(MouseEvent e) {
				/* do nothing */
			}
		};

		shell.addMouseListener(shellMouseListener);
	}

//	private void createHWKeyRegion() {
//		if (compositeBase != null) {
//			compositeBase.dispose();
//			compositeBase = null;
//		}
//
//		List<KeyMapType> keyMapList =
//				SkinUtil.getHWKeyMapList(currentState.getCurrentRotationId());
//
//		if (keyMapList != null && keyMapList.isEmpty() == false) {
//			compositeBase = new Composite(shell, SWT.NONE);
//			compositeBase.setLayout(new GridLayout(1, true));
//
//			for (KeyMapType keyEntry : keyMapList) {
//				Button hardKeyButton = new Button(compositeBase, SWT.FLAT);
//				hardKeyButton.setText(keyEntry.getEventInfo().getKeyName());
//				hardKeyButton.setToolTipText(keyEntry.getTooltip());
//
//				hardKeyButton.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
//
//				final int keycode = keyEntry.getEventInfo().getKeyCode();
//				hardKeyButton.addMouseListener(new MouseListener() {
//					@Override
//					public void mouseDown(MouseEvent e) {
//						KeyEventData keyEventData = new KeyEventData(
//								KeyEventType.PRESSED.value(), keycode, 0, 0);
//						communicator.sendToQEMU(SendCommand.SEND_HARD_KEY_EVENT, keyEventData);
//					}
//
//					@Override
//					public void mouseUp(MouseEvent e) {
//						KeyEventData keyEventData = new KeyEventData(
//								KeyEventType.RELEASED.value(), keycode, 0, 0);
//						communicator.sendToQEMU(SendCommand.SEND_HARD_KEY_EVENT, keyEventData);
//					}
//
//					@Override
//					public void mouseDoubleClick(MouseEvent e) {
//						/* do nothing */
//					}
//				});
//			}
//
//			FormData dataComposite = new FormData();
//			dataComposite.left = new FormAttachment(lcdCanvas, 0);
//			dataComposite.top = new FormAttachment(0, 0);
//			compositeBase.setLayoutData(dataComposite);
//		}
//	}

	@Override
	public void composerFinalize() {
		if (null != shellPaintListener) {
			shell.removePaintListener(shellPaintListener);
		}

		if (null != shellMouseMoveListener) {
			shell.removeMouseMoveListener(shellMouseMoveListener);
		}

		if (null != shellMouseListener) {
			shell.removeMouseListener(shellMouseListener);
		}

		frame.freePatches();
	}
}
