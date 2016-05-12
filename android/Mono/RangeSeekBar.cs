using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using Android.App;
using Android.Content;
using Android.Graphics;
using Android.OS;
using Android.Runtime;
using Android.Views;
using Android.Widget;

using Java.Lang;

namespace android {
	class RangeSeekBar<T> : ImageView where T : Number {
		private Paint paint = new Paint(PaintFlags.AntiAlias);
		private Bitmap thumbImageLeft;
        private Bitmap thumbImageRight;
        private Bitmap thumbPressedImageLeft;
        private Bitmap thumbPressedImageRight;
        private float thumbWidth;
		private float thumbHalfWidth;
		private float thumbHalfHeight;
		private float lineHeight;
		private float padding;
		private T absoluteMinValue, absoluteMaxValue;
        private NumberType numberType;
        private double absoluteMinValuePrim, absoluteMaxValuePrim;
		private double normalizedMinValue = 0d;
		private double normalizedMaxValue = 1d;
		private Thumb pressedThumb = Thumb.NULL;
		private bool notifyWhileDragging = false;
		private IOnRangeSeekBarChangeListener<T> listener;

		/**
         * Default color of a {@link RangeSeekBar}, #FF33B5E5. This is also known as "Ice Cream Sandwich" blue.
         */
		public static int DEFAULT_COLOR = Color.Argb(0xFF, 0x33, 0xB5, 0xE5);

		/**
         * An invalid pointer id.
         */
		public static int INVALID_POINTER_ID = 255;

		// Localized constants from MotionEvent for compatibility
		// with API < 8 "Froyo".
		public static int ACTION_POINTER_UP = 0x6, ACTION_POINTER_INDEX_MASK = 0x0000ff00, ACTION_POINTER_INDEX_SHIFT = 8;

		private float mDownMotionX;
		private int mActivePointerId = INVALID_POINTER_ID;

		/**
         * On touch, this offset plus the scaled value from the position of the touch will form the progress value. Usually 0.
         */
		float mTouchProgressOffset;

		private int mScaledTouchSlop;
		private bool mIsDragging;

		public RangeSeekBar(T absoluteMinValue, T absoluteMaxValue, Context context) : base(context) {
			this.absoluteMinValue = absoluteMinValue;
			this.absoluteMaxValue = absoluteMaxValue;
			this.absoluteMinValuePrim = absoluteMinValue.DoubleValue();
			this.absoluteMaxValuePrim = absoluteMaxValue.DoubleValue();
			this.numberType = this.fromNumber(absoluteMinValue);

			// make RangeSeekBar focusable. This solves focus handling issues in case EditText widgets are being used along with the RangeSeekBar within ScollViews.
			this.Focusable = true;
			this.FocusableInTouchMode = true;
			this.init();
			this.padding = this.thumbHalfWidth;
		}
		public RangeSeekBar(T absoluteMinValue, T absoluteMaxValue, float padding, Context context) : base(context) {
			this.absoluteMinValue = absoluteMinValue;
			this.absoluteMaxValue = absoluteMaxValue;
			this.absoluteMinValuePrim = absoluteMinValue.DoubleValue();
			this.absoluteMaxValuePrim = absoluteMaxValue.DoubleValue();
			this.numberType = this.fromNumber(absoluteMinValue);

			// make RangeSeekBar focusable. This solves focus handling issues in case EditText widgets are being used along with the RangeSeekBar within ScollViews.
			this.Focusable = true;
			this.FocusableInTouchMode = true;
			this.padding = padding;
			this.init();
		}
		private void init() {
			thumbImageLeft = BitmapFactory.DecodeResource(Resources, Resource.Drawable.select_left);
			thumbImageRight = BitmapFactory.DecodeResource(Resources, Resource.Drawable.select_right);
			thumbPressedImageLeft = BitmapFactory.DecodeResource(Resources, Resource.Drawable.select_left);
			thumbPressedImageRight = BitmapFactory.DecodeResource(Resources, Resource.Drawable.select_right);
			thumbWidth = thumbImageLeft.Width;
			thumbHalfWidth = 0.5f * thumbWidth;
			thumbHalfHeight = 0.5f * thumbImageLeft.Height;
			lineHeight = 0.1337f * thumbHalfHeight;
			mScaledTouchSlop = ViewConfiguration.Get(Context).ScaledTouchSlop;
		}
		public bool isNotifyWhileDragging() {
			return notifyWhileDragging;
		}

		/**
         * Should the widget notify the listener callback while the user is still dragging a thumb? Default is false.
         * 
         * @param flag
         */
		public void setNotifyWhileDragging(bool flag) {
			this.notifyWhileDragging = flag;
		}

		/**
         * Returns the absolute minimum value of the range that has been set at construction time.
         * 
         * @return The absolute minimum value of the range.
         */
		public T getAbsoluteMinValue() {
			return absoluteMinValue;
		}

		/**
         * Returns the absolute maximum value of the range that has been set at construction time.
         * 
         * @return The absolute maximum value of the range.
         */
		public T getAbsoluteMaxValue() {
			return absoluteMaxValue;
		}

		/**
         * Returns the currently selected min value.
         * 
         * @return The currently selected min value.
         */
		public T getSelectedMinValue() {
			return normalizedToValue(normalizedMinValue);
		}
		public void setSelectedMinValue(T value) {
			// in case absoluteMinValue == absoluteMaxValue, avoid division by zero when normalizing.
			if (0 == (absoluteMaxValuePrim - absoluteMinValuePrim)) {
				setNormalizedMinValue(0d);
			} else {
				setNormalizedMinValue(valueToNormalized(value));
			}
		}

		/**
         * Returns the currently selected max value.
         * 
         * @return The currently selected max value.
         */
		public T getSelectedMaxValue() {
			return normalizedToValue(normalizedMaxValue);
		}

		/**
         * Sets the currently selected maximum value. The widget will be invalidated and redrawn.
         * 
         * @param value
         *            The Number value to set the maximum value to. Will be clamped to given absolute minimum/maximum range.
         */
		public void setSelectedMaxValue(T value) {
			// in case absoluteMinValue == absoluteMaxValue, avoid division by zero when normalizing.
			if (0 == (absoluteMaxValuePrim - absoluteMinValuePrim)) {
				setNormalizedMaxValue(1d);
			} else {
				setNormalizedMaxValue(valueToNormalized(value));
			}
		}

		/**
         * Registers given listener callback to notify about changed selected values.
         * 
         * @param listener
         *            The listener to notify about changed selected values.
         */
		public void setOnRangeSeekBarChangeListener(IOnRangeSeekBarChangeListener<T> listener) {
			this.listener = listener;
		}
		/**
         * Handles thumb selection and movement. Notifies listener callback on certain events.
         */
		public override bool OnTouchEvent(MotionEvent ev) {

			if (!this.Enabled)
				return false;

			int pointerIndex;

			var action = ev.Action;
			switch (action & MotionEventActions.Mask) {

			case MotionEventActions.Down:
				// Remember where the motion event started
				mActivePointerId = ev.GetPointerId(ev.PointerCount - 1);
				pointerIndex = ev.FindPointerIndex(mActivePointerId);
				this.mDownMotionX = ev.GetX(pointerIndex);

				this.pressedThumb = evalPressedThumb(mDownMotionX);

				// Only handle thumb presses.
				if (pressedThumb == Thumb.NULL)
					return base.OnTouchEvent(ev);

				this.Pressed = true;
				this.Invalidate();
				this.onStartTrackingTouch();
				this.trackTouchEvent(ev);
				this.attemptClaimDrag();

				break;
			case MotionEventActions.Move:
				if (pressedThumb != null) {

					if (mIsDragging) {
						this.trackTouchEvent(ev);
					} else {
						// Scroll to follow the motion event
						pointerIndex = ev.FindPointerIndex(mActivePointerId);
						float x = ev.GetX(pointerIndex);

						if (Java.Lang.Math.Abs(x - this.mDownMotionX) > this.mScaledTouchSlop) {
							this.Pressed = true;
							this.Invalidate();
							this.onStartTrackingTouch();
							this.trackTouchEvent(ev);
							this.attemptClaimDrag();
						}
					}

					if (this.notifyWhileDragging && this.listener != null) {
						this.listener.onRangeSeekBarValuesChanged(this, this.getSelectedMinValue(), this.getSelectedMaxValue());
					}
				}
				break;
			case MotionEventActions.Up:
				if (mIsDragging) {

					this.trackTouchEvent(ev);

					this.onStopTrackingTouch();

					this.Pressed = false;
				} else {
					// Touch up when we never crossed the touch slop threshold
					// should be interpreted as a tap-seek to that location.
					this.onStartTrackingTouch();

					this.trackTouchEvent(ev);

					this.onStopTrackingTouch();
				}

				this.pressedThumb = Thumb.NULL;

				this.Invalidate();
				if (this.listener != null) {
					this.listener.onRangeSeekBarValuesChanged(this, this.getSelectedMinValue(), this.getSelectedMaxValue());
				}
				break;
			case MotionEventActions.PointerDown: {
					int index = ev.PointerCount - 1;
					// final int index = ev.getActionIndex();
					this.mDownMotionX = ev.GetX(index);
					this.mActivePointerId = ev.GetPointerId(index);
					this.Invalidate();
					break;
				}
			case MotionEventActions.PointerUp:

				this.onSecondaryPointerUp(ev);

				this.Invalidate();
				break;
			case MotionEventActions.Cancel:
				if (mIsDragging) {

					this.onStopTrackingTouch();

					this.Pressed = false;
				}

				this.Invalidate(); // see above explanation
				break;
			}
			return true;
		}
		private void onSecondaryPointerUp(MotionEvent ev) {
			int pointerIndex = ((int)ev.Action & ACTION_POINTER_INDEX_MASK) >> ACTION_POINTER_INDEX_SHIFT;

			int pointerId = ev.GetPointerId(pointerIndex);
			if (pointerId == mActivePointerId) {
				// This was our active pointer going up. Choose
				// a new active pointer and adjust accordingly.
				// TODO: Make this decision more intelligent.
				int newPointerIndex = pointerIndex == 0 ? 1 : 0;
				mDownMotionX = ev.GetX(newPointerIndex);
				mActivePointerId = ev.GetPointerId(newPointerIndex);
			}
		}

		private void trackTouchEvent(MotionEvent ev) {
			int pointerIndex = ev.FindPointerIndex(mActivePointerId);
			float x = ev.GetX(pointerIndex);

			if (Thumb.MIN.Equals(pressedThumb)) {
				this.setNormalizedMinValue(screenToNormalized(x));
			} else if (Thumb.MAX.Equals(pressedThumb)) {
				this.setNormalizedMaxValue(screenToNormalized(x));
			}
		}

		/**
         * Tries to claim the user's drag motion, and requests disallowing any ancestors from stealing events in the drag.
         */
		private void attemptClaimDrag() {
			if (this.Parent != null) {
				this.Parent.RequestDisallowInterceptTouchEvent(true);
			}
		}

		/**
         * This is called when the user has started touching this widget.
         */
		void onStartTrackingTouch() {
			mIsDragging = true;
		}

		/**
         * This is called when the user either releases his touch or the touch is canceled.
         */
		void onStopTrackingTouch() {
			mIsDragging = false;
		}

		/**
         * Ensures correct size of the widget.
         */
		protected override void OnMeasure(int widthMeasureSpec, int heightMeasureSpec) {
			int width = 200;
			if (MeasureSpecMode.Unspecified != MeasureSpec.GetMode(widthMeasureSpec)) {
				width = MeasureSpec.GetSize(widthMeasureSpec);
			}
			int height = thumbImageLeft.Height;
			if (MeasureSpecMode.Unspecified != MeasureSpec.GetMode(heightMeasureSpec)) {
				height = Java.Lang.Math.Min(height, MeasureSpec.GetSize(heightMeasureSpec));
			}
			this.SetMeasuredDimension(width, height);
		}

		/**
         * Draws the widget on the given canvas.
         */
		protected override void OnDraw(Canvas canvas) {
			base.OnDraw(canvas);

			// draw seek bar background line
			/*                final RectF rectbg = new RectF(padding, 0.5f * (getHeight() - lineHeight / 3.0f), getWidth() - padding, 0.5f * (getHeight() + lineHeight / 3.0f));
							paint.setStyle(Style.FILL);
							paint.setColor(Color.GRAY);
							paint.setAntiAlias(true);
							canvas.drawRect(rectbg, paint);

							final RectF rectfg = new RectF(padding, 0.5f * (getHeight() - lineHeight), getWidth() - padding, 0.5f * (getHeight() + lineHeight));
							// draw seek bar active range line
							rectfg.left = normalizedToScreen(normalizedMinValue);
							rectfg.right = normalizedToScreen(normalizedMaxValue);

							// custom red color
							paint.setColor(Color.argb(0xFF, 0xC3, 0x00, 0x00));
							canvas.drawRect(rectfg, paint);*/

			// draw minimum thumb
			this.drawThumbLeft(normalizedToScreen(normalizedMinValue), Thumb.MIN.Equals(pressedThumb), canvas);

			// draw maximum thumb
			this.drawThumbRight(normalizedToScreen(normalizedMaxValue), Thumb.MAX.Equals(pressedThumb), canvas);
		}

		/**
         * Overridden to save instance state when device orientation changes. This method is called automatically if you assign an id to the RangeSeekBar widget using the {@link #setId(int)} method. Other members of this class than the normalized min and max values don't need to be saved.
         */
		protected override IParcelable OnSaveInstanceState() {
			Bundle bundle = new Bundle();
			bundle.PutParcelable("SUPER", base.OnSaveInstanceState());
			bundle.PutDouble("MIN", this.normalizedMinValue);
			bundle.PutDouble("MAX", this.normalizedMaxValue);
			return bundle;
		}
		protected override void OnRestoreInstanceState(IParcelable parcel) {
			Bundle bundle = (Bundle)parcel;
			base.OnRestoreInstanceState((IParcelable)bundle.GetParcelable("SUPER"));
			this.normalizedMinValue = bundle.GetDouble("MIN");
			this.normalizedMaxValue = bundle.GetDouble("MAX");
		}
		/**
         * Draws the "normal" resp. "pressed" thumb image on specified x-coordinate.
         * 
         * @param screenCoord
         *            The x-coordinate in screen space where to draw the image.
         * @param pressed
         *            Is the thumb currently in "pressed" state?
         * @param canvas
         *            The canvas to draw upon.
         */
		private void drawThumbLeft(float screenCoord, bool pressed, Canvas canvas) {
			canvas.DrawBitmap(pressed ? thumbPressedImageLeft : thumbImageLeft, screenCoord - thumbHalfWidth, (float)((0.5f * this.Height) - thumbHalfHeight), paint);
		}
		private void drawThumbRight(float screenCoord, bool pressed, Canvas canvas) {
			canvas.DrawBitmap(pressed ? thumbPressedImageRight : thumbImageRight, screenCoord - thumbHalfWidth, (float)((0.5f * this.Height) - thumbHalfHeight), paint);
		}
		/**
         * Decides which (if any) thumb is touched by the given x-coordinate.
         * 
         * @param touchX
         *            The x-coordinate of a touch event in screen space.
         * @return The pressed thumb or null if none has been touched.
         */
		private Thumb evalPressedThumb(float touchX) {
			Thumb result = Thumb.NULL;
			bool minThumbPressed = this.isInThumbRange(touchX, normalizedMinValue);
			bool maxThumbPressed = this.isInThumbRange(touchX, normalizedMaxValue);
			if (minThumbPressed && maxThumbPressed) {
				// if both thumbs are pressed (they lie on top of each other), choose the one with more room to drag. this avoids "stalling" the thumbs in a corner, not being able to drag them apart anymore.
				result = (touchX / this.Width > 0.5f) ? Thumb.MIN : Thumb.MAX;
			} else if (minThumbPressed) {
				result = Thumb.MIN;
			} else if (maxThumbPressed) {
				result = Thumb.MAX;
			}
			return result;
		}
		/**
         * Decides if given x-coordinate in screen space needs to be interpreted as "within" the normalized thumb x-coordinate.
         * 
         * @param touchX
         *            The x-coordinate in screen space to check.
         * @param normalizedThumbValue
         *            The normalized x-coordinate of the thumb to check.
         * @return true if x-coordinate is in thumb range, false otherwise.
         */
		private bool isInThumbRange(float touchX, double normalizedThumbValue) {
			return Java.Lang.Math.Abs(touchX - this.normalizedToScreen(normalizedThumbValue)) <= thumbHalfWidth;
		}

		/**
         * Sets normalized min value to value so that 0 <= value <= normalized max value <= 1. The View will get invalidated when calling this method.
         * 
         * @param value
         *            The new normalized min value to set.
         */
		public void setNormalizedMinValue(double value) {
			normalizedMinValue = Java.Lang.Math.Max(0d, Java.Lang.Math.Min(1d, Java.Lang.Math.Min(value, normalizedMaxValue)));
			this.Invalidate();
		}

		/**
         * Sets normalized max value to value so that 0 <= normalized min value <= value <= 1. The View will get invalidated when calling this method.
         * 
         * @param value
         *            The new normalized max value to set.
         */
		public void setNormalizedMaxValue(double value) {
			normalizedMaxValue = Java.Lang.Math.Max(0d, Java.Lang.Math.Min(1d, Java.Lang.Math.Max(value, normalizedMinValue)));
			this.Invalidate();
		}

		/**
         * Converts a normalized value to a Number object in the value space between absolute minimum and maximum.
         * 
         * @param normalized
         * @return
         */
		private T normalizedToValue(double normalized) {
			return (T)toNumber(absoluteMinValuePrim + normalized * (absoluteMaxValuePrim - absoluteMinValuePrim));
		}

		/**
         * Converts the given Number value to a normalized double.
         * 
         * @param value
         *            The Number value to normalize.
         * @return The normalized double.
         */
		private double valueToNormalized(T value) {
			if (0 == absoluteMaxValuePrim - absoluteMinValuePrim) {
				// prevent division by zero, simply return 0.
				return 0d;
			}
			return (value.DoubleValue() - absoluteMinValuePrim) / (absoluteMaxValuePrim - absoluteMinValuePrim);
		}
		/**
         * Converts a normalized value into screen space.
         * 
         * @param normalizedCoord
         *            The normalized value to convert.
         * @return The converted value in screen space.
         */
		private float normalizedToScreen(double normalizedCoord) {
			return (float)(padding + normalizedCoord * (this.Width - 2 * padding));
		}

		/**
         * Converts screen space x-coordinates into normalized values.
         * 
         * @param screenCoord
         *            The x-coordinate in screen space to convert.
         * @return The normalized value.
         */
		private double screenToNormalized(float screenCoord) {
			int width = this.Width;
			if (width <= 2 * padding) {
				// prevent division by zero, simply return 0.
				return 0d;
			} else {
				double result = (screenCoord - padding) / (width - 2 * padding);
				return Java.Lang.Math.Min(1d, Java.Lang.Math.Max(0d, result));
			}
		}

		/**
         * Callback listener interface to notify about changed range values.
         * 
         * @author Stephan Tittel (stephan.tittel@kom.tu-darmstadt.de)
         * 
         * @param <T>
         *            The Number type the RangeSeekBar has been declared with.
         */
		public interface IOnRangeSeekBarChangeListener<TT> where TT : Number {
			void onRangeSeekBarValuesChanged(RangeSeekBar<TT> bar, T minValue, T maxValue);
		}
		/**
         * Thumb constants (min and max).
         */
		private enum Thumb {
			MIN, MAX, NULL
		};
		/**
         * Utility enumaration used to convert between Numbers and doubles.
         * 
         * @author Stephan Tittel (stephan.tittel@kom.tu-darmstadt.de)
         * 
         */
		private enum NumberType {
			LONG, DOUBLE, INTEGER, FLOAT, SHORT, BYTE, BIG_DECIMAL

		}


		private NumberType fromNumber(T value) {
			if (value is Long) {
				return NumberType.LONG;
			}
			if (value is Java.Lang.Double) {
				return NumberType.DOUBLE;
			}
			if (value is Integer) {
				return NumberType.INTEGER;
			}
			if (value is Float) {
				return NumberType.FLOAT;
			}
			if (value is Short) {
				return NumberType.SHORT;
			}
			if (value is Java.Lang.Byte) {
				return NumberType.BYTE;
			}
			if (value is Java.Math.BigDecimal) {
				return NumberType.BIG_DECIMAL;
			}
			throw new IllegalArgumentException("Number class '" + this.Class.Name + "' is not supported");
		}

		public Number toNumber(double value) {
			switch (this.numberType) {
			case NumberType.LONG:
				return new Long((long)value);
			case NumberType.DOUBLE:
				return new Java.Lang.Double(value);
			case NumberType.INTEGER:
				return new Integer((int)value);
			case NumberType.FLOAT:
				return new Float(value);
			case NumberType.SHORT:
				return new Short((short)value);
			case NumberType.BYTE:
				return new Java.Lang.Byte((sbyte)value);
			case NumberType.BIG_DECIMAL:
				return new Java.Math.BigDecimal(value);
			}
			throw new InstantiationError("can't convert " + this + " to a Number object");
		}
	}
}