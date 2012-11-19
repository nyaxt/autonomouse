package com.autonomouse;

import java.io.IOException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialProber;
import com.hoho.android.usbserial.util.SerialInputOutputManager;

import android.app.Activity;
import android.content.Context;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.ScrollView;
import android.widget.TextView;

public class MainActivity extends Activity {
  private final String TAG = MainActivity.class.getSimpleName();
  /**
   * The device currently in use, or {@code null}.
   */
  private UsbSerialDriver mSerialDevice;

  /**
   * The system's USB service.
   */
  private UsbManager mUsbManager;

  private TextView mDumpTextView;
  private TextView mTitleTextView;
  private ScrollView mScrollView;
  private Button mClickTestButton;
  private Button mScrollTestButton;
  private Button mResetBluetoothButton;
  private Button mMoveMouseUp;
  private Button mMoveMouseDown;
  private Button mMoveMouseLeft;
  private Button mMoveMouseRight;

  private final ExecutorService mExecutor = Executors.newSingleThreadExecutor();

  private SerialInputOutputManager mSerialIoManager;

  private final SerialInputOutputManager.Listener mListener = new SerialInputOutputManager.Listener() {

    @Override
    public void onRunError(Exception e) {
      Log.d(TAG, "Runner stopped.");
    }

    @Override
    public void onNewData(final byte[] data) {
      MainActivity.this.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          MainActivity.this.updateReceivedData(data);
        }
      });
    }
  };

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    mUsbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
    mDumpTextView = (TextView) findViewById(R.id.statusTextView);
    mTitleTextView = (TextView) findViewById(R.id.titleTextView);
    mScrollView = (ScrollView) findViewById(R.id.scrollView1);
    mClickTestButton = (Button) findViewById(R.id.button1);
    mClickTestButton.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View arg0) {
        if (mSerialIoManager != null) {
          try {
            mSerialDevice.write(new byte[] { 97 }, 1000);
          } catch (IOException e) {
          }
        }
      }
    });
    mScrollTestButton = (Button) findViewById(R.id.button2);
    mScrollTestButton.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View arg0) {
        if (mSerialIoManager != null) {
          try {
            mSerialDevice.write(new byte[] { 98 }, 1000);
          } catch (IOException e) {
          }
        }
      }
    });
    mResetBluetoothButton = (Button) findViewById(R.id.button3);
    mResetBluetoothButton.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View arg0) {
        if (mSerialIoManager != null) {
          try {
            mSerialDevice.write(new byte[] { 100 }, 1000);
          } catch (IOException e) {
          }
        }
      }
    });

    mMoveMouseUp = (Button) findViewById(R.id.button4);
    mMoveMouseUp.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View arg0) {
        if (mSerialIoManager != null) {
          try {
            mSerialDevice.write(new byte[] { 49 }, 1000);
          } catch (IOException e) {
          }
        }
      }
    });
    mMoveMouseDown = (Button) findViewById(R.id.button5);
    mMoveMouseDown.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View arg0) {
        if (mSerialIoManager != null) {
          try {
            mSerialDevice.write(new byte[] { 50 }, 1000);
          } catch (IOException e) {
          }
        }
      }
    });
    mMoveMouseLeft = (Button) findViewById(R.id.button6);
    mMoveMouseLeft.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View arg0) {
        if (mSerialIoManager != null) {
          try {
            mSerialDevice.write(new byte[] { 51 }, 1000);
          } catch (IOException e) {
          }
        }
      }
    });
    mMoveMouseRight = (Button) findViewById(R.id.button7);
    mMoveMouseRight.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View arg0) {
        if (mSerialIoManager != null) {
          try {
            mSerialDevice.write(new byte[] { 52 }, 1000);
          } catch (IOException e) {
          }
        }
      }
    });
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    getMenuInflater().inflate(R.menu.activity_main, menu);
    return true;
  }

  @Override
  protected void onPause() {
    super.onPause();
    stopIoManager();
    if (mSerialDevice != null) {
      try {
        mSerialDevice.close();
      } catch (IOException e) {
        // Ignore.
      }
      mSerialDevice = null;
    }
  }

  @Override
  protected void onResume() {
    super.onResume();
    mSerialDevice = UsbSerialProber.acquire(mUsbManager);
    Log.d(TAG, "Resumed, mSerialDevice=" + mSerialDevice);
    if (mSerialDevice == null) {
      mTitleTextView.setText("No serial device.");
    } else {
      try {
        mSerialDevice.open();
      } catch (IOException e) {
        Log.e(TAG, "Error setting up device: " + e.getMessage(), e);
        mTitleTextView.setText("Error opening device: " + e.getMessage());
        try {
          mSerialDevice.close();
        } catch (IOException e2) {
          // Ignore.
        }
        mSerialDevice = null;
        return;
      }
      mTitleTextView.setText("Attached to serial device");
    }
    onDeviceStateChange();
  }

  private void stopIoManager() {
    if (mSerialIoManager != null) {
      Log.i(TAG, "Stopping io manager ..");
      mSerialIoManager.stop();
      mSerialIoManager = null;
    }
  }

  private void startIoManager() {
    if (mSerialDevice != null) {
      Log.i(TAG, "Starting io manager ..");
      mSerialIoManager = new SerialInputOutputManager(mSerialDevice, mListener);
      mExecutor.submit(mSerialIoManager);
    }
  }

  private void onDeviceStateChange() {
    stopIoManager();
    startIoManager();
  }

  private void updateReceivedData(byte[] data) {
    final String message = new String(data);
    mDumpTextView.append(message);
    mScrollView.smoothScrollTo(0, mDumpTextView.getBottom());
  }
}
