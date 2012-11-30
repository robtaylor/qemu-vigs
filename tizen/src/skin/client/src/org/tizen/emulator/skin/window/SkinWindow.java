/**
 *
 *
 * Copyright ( C ) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or ( at your option ) any later version.
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

package org.tizen.emulator.skin.window;

import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.log.SkinLogger;

public class SkinWindow {
	protected Logger logger = SkinLogger.getSkinLogger(
			SkinWindow.class).getLogger();

	protected Shell shell;
	protected Shell parent;
	private int shellPositionType;

	public SkinWindow(Shell parent, int shellPositionType) {
		this.parent = parent;
		this.shellPositionType = shellPositionType;
	}

	public Shell getShell() {
		return shell;
	}

	public void open() {
		if (shell.isDisposed()) {
			return;
		}

		setShellPosition();
		shell.open();

		while (!shell.isDisposed()) {
			if (!shell.getDisplay().readAndDispatch()) {
				shell.getDisplay().sleep();
			}
		}
	}

	protected void setShellPosition() {
		int x = 0, y = 0;

		if (shellPositionType == (SWT.RIGHT | SWT.TOP)) {
			Rectangle monitorBound = Display.getDefault().getBounds();
			logger.info("host monitor display bound : " + monitorBound);
			Rectangle emulatorBound = parent.getBounds();
			logger.info("current Emulator window bound : " + emulatorBound);
			Rectangle panelBound = shell.getBounds();
			logger.info("current Panel shell bound : " + panelBound);

			/* location correction */
			x = emulatorBound.x + emulatorBound.width;
			y = emulatorBound.y;
			if ((x + panelBound.width) > (monitorBound.x + monitorBound.width)) {
				x = emulatorBound.x - panelBound.width;
			}
		} else { /* SWT.RIGHT | SWT.CENTER */
			x = parent.getBounds().x + parent.getBounds().width;
			y = parent.getBounds().y + (parent.getBounds().height / 2) -
					(shell.getSize().y / 2);
		}

		shell.setLocation(x, y);
	}
}
