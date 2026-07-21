# advc.092: New module to help specify positions of rectangular widgets
# relative to each other.

# (Not naming this "Point2D" to avoid potential clashes with other modules)
class PointLayout:
	def __init__(self, fX, fY):
		self.fX = fX
		self.fY = fY
	def x(self):
		return self.fX
	def y(self):
		return self.fY
	def move(self, dX, dY):
		self.fX += dX
		self.fY += dY
	def __str__(self):
		return ( "Point(" + str(self.fX) + ", " + str(self.fY) + ")" )

# Layout data of a rectangular widget or group of widgets:
# a pair of coordinates and a pair of side lengths.
# The constructor receives the coordinates as an offset from another
# ("parent") RectLayout - and converts them to absolute screen coordinates
# accessible through the x and y methods.
# For convenience, the constructor works with float values, but the accessors
# return integers because that's what most of the CyGInterfaceScreen functions
# in the EXE expect.
class RectLayout(object):
	_RESERVED_CONST = 10000
	_ALIGN_CENTER = _RESERVED_CONST
	_ALIGN_OPPOSITE = _RESERVED_CONST + 1
	_MAX_LENGTH =  2 * _RESERVED_CONST

	LEFT = 0
	TOP = 0
	CENTER = _ALIGN_CENTER	
	RIGHT = _ALIGN_OPPOSITE
	BOTTOM = _ALIGN_OPPOSITE
	MAX = _MAX_LENGTH
	def __init__(self,
			# ('None' is allowed only for the RectLayout representing the screen itself.
			# In that case, fX and fY need to be absolute screen coordinates.)
			lParent,
			# Distance from lParent's left edge if positive,
			# from lParent's right edge if negative.
			# CENTER centers this widget horizontally.
			# RIGHT aligns this widget horizontally to the right edge of lParent.
			# LEFT is equivalent to fX=0.
			fX,
			# Distance from lParent's top edge if positive,
			# from lParent's bottom edge if negative.
			# CENTER centers this widget vertically.
			# BOTTOM aligns this widget vertically to the bottom edge of lParent.
			# TOP is equivalent to 0.
			fY,
			# Our width in pixels if positive.
			# If negative, then fWidth is the distance from lParent's right edge
			# and, when fX is set to CENTER, then negative fWidth is also the 
			# distance from lParent's left edge. Negative fWidth can't be combined
			# with RIGHT alignment.
			fWidth,
			# Our height in pixels if positive.
			# If negative, then fHeight is the distance from lParent's bottom edge
			# and, when fY is set to CENTER, then negative fHeight is also the 
			# distance from lParent's top edge. Negative fHeight can't be combined
			# with BOTTOM alignment.
			fHeight,
			bOffScreen = False):
		if not lParent:
			self.fX = fX
			self.fY = fY
			fPrelimWidth = fWidth
			fPrelimHeight = fHeight
		else:
			fPrelimWidth = RectLayout._calcSideLenPrelim(lParent.width(), fWidth)
			fPrelimHeight = RectLayout._calcSideLenPrelim(lParent.height(), fHeight)
			self.fX = RectLayout._calcCoord(lParent.x(), lParent.width(),
					fX, fPrelimWidth, bOffScreen)
			self.fY = RectLayout._calcCoord(lParent.y(), lParent.height(),
					fY, fPrelimHeight, bOffScreen)
		self.fWidth = fPrelimWidth
		if fWidth == RectLayout.MAX:
			self.fWidth = min(self.fWidth, lParent.xRight() - self.fX)
		elif fWidth < 0:
			if fX == RectLayout.CENTER: # Margin applies to both edges
				self.fWidth += fWidth
				self.fX -= fWidth / 2
			else:
				assert fX < RectLayout._RESERVED_CONST
				self.fWidth -= self.fX - lParent.x() # Subtract left margin
		self.fHeight = fPrelimHeight
		if fHeight == RectLayout.MAX:
			self.fHeight = min(self.fHeight, lParent.yBottom() - self.fY)
		elif fHeight < 0:
			if fY == RectLayout.CENTER: # Margin applies to both edges
				self.fHeight += fHeight
				self.fY -= fHeight / 2
			else:
				assert fY < RectLayout._RESERVED_CONST
				self.fHeight -= self.fY - lParent.y() # Subtract left margin
		assert 0 <= self.fWidth < RectLayout._RESERVED_CONST
		assert 0 <= self.fHeight < RectLayout._RESERVED_CONST
		assert self.fX < RectLayout._RESERVED_CONST
		assert self.fY < RectLayout._RESERVED_CONST
		# (It's OK for widgets to be only partly on the screen, so
		# having negative fX or fY isn't necessarily wrong.)

	def copy(self):
		lCopy = RectLayout(None, 0, 0, 0, 0)
		lCopy.fX = self.fX
		lCopy.fY = self.fY
		lCopy.fWidth = self.fWidth
		lCopy.fHeight = self.fHeight
		return lCopy

	# Preliminary calculation of side length. Preliminary b/c our screen coordinates
	# haven't been calculated yet.
	@staticmethod # (Very much not supposed to access self)
	def _calcSideLenPrelim(iParentSideLen, fSideLen):
		if fSideLen == RectLayout.MAX:
			return iParentSideLen
		else:
			fR = fSideLen
			if fSideLen < 0:
				fR += iParentSideLen
			return fR

	# Calculate absolute coordinate on screen
	@staticmethod
	def _calcCoord(iParentVal, iParentDim, iVal, iDim, bOffScreen):
		iR = iParentVal
		if iVal == RectLayout.CENTER:
			iR += (iParentDim - iDim) / 2
			return iR
		if iVal == RectLayout._ALIGN_OPPOSITE:
			iR += iParentDim - iDim
			return iR
		iR += iVal
		if iVal < 0 and not bOffScreen:
			iR += iParentDim - iDim
		return iR
	def move(self, fXChange, fYChange):
		self.fX += fXChange
		self.fY += fYChange
	def moveTo(self, fX, fY):
		self.fX = fX
		self.fY = fY
	def adjustSize(self, fWidthChange, fHeightChange):
		self.fWidth += fWidthChange
		self.fHeight += fHeightChange
	# Screen coordinates of our upper left corner
	def x(self):
		return int(round(self.fX))
	def y(self):
		return int(round(self.fY))
	def upperLeft(self):
		return PointLayout(self.fX, self.fY)
	# Screen coordinates of lower right corner
	def xRight(self):
		return self.x() + self.width()
	def yBottom(self):
		return self.y() + self.height()
	# Screen coordinates of center
	def xCenter(self):
		return int(round(self.fX + self.fWidth / 2))
	def yCenter(self):
		return int(round(self.fY + self.fHeight / 2))
	# Our side lengths (pixels)
	def width(self):
		return int(round(self.fWidth))
	def height(self):
		return int(round(self.fHeight))
	def encloses(self, lOther):
		return (self.x() <= lOther.x() and self.xRight() >= lOther.xRight() and
				self.y() <= lOther.y() and self.yBottom() >= lOther.yBottom())
	def __str__(self):
		return ( "Rect(" +
				str(self.fX) + ", " + str(self.fY) + ", " +
				str(self.fWidth) + ", " + str(self.fHeight) + ")" )
	@staticmethod
	def offsetPoint(lRect, fDeltaX, fDeltaY = 0):
		if fDeltaX == RectLayout.CENTER:
			fDeltaX = lRect.fWidth / 2
		if fDeltaX == RectLayout.RIGHT:
			fDeltaX = lRect.fWidth
		if fDeltaY == RectLayout.CENTER:
			fDeltaY = lRect.fHeight / 2
		if fDeltaY == RectLayout.BOTTOM:
			fDeltaY = lRect.fHeight
		assert fDeltaX < RectLayout._RESERVED_CONST
		assert fDeltaY < RectLayout._RESERVED_CONST
		return PointLayout(lRect.fX + fDeltaX, lRect.fY + fDeltaY)

class SquareLayout(RectLayout):
	def __init__(self, lParent, fX, fY, iSize, bOffScreen = False):
		super(SquareLayout, self).__init__(lParent, fX, fY, iSize, iSize, bOffScreen)
	def size(self):
		return self.width()

# Positional data for a group of widgets layed out in a single row or column.
# Intended as an abstract class.
class _SingleFileLayout(RectLayout):
	HORIZONTAL = True
	VERTICAL = False
	def __init__(self, lParent,
			# Whether the widgets are layed out horizontally (HORIZONTAL)
			# or vertically (VERTICAL)
			orientation,
			fX, fY, # See super class ctor
			# (Positive) number of widgets in the group.
			# RectLayout.MAX for the highest number that will (fully) fit
			# into lParent (but at least 1).
			iWidgets,
			# Space in between two adjacent widgets.
			# Negative space means that they overlap.
			iSpacing,
			# (Positive) widget size. By default, square dimensions are assumed.
			fWidth, fHeight = None):
		if fHeight is None:
			fHeight = fWidth
		bHorizontal = orientation
		if bHorizontal:
			iExpandingLen = fWidth
			iTotalHeight = fHeight
		else:
			iExpandingLen = fHeight
			iTotalWidth = fWidth
		if iWidgets == RectLayout.MAX:
			if bHorizontal:
				iParentLen = lParent.width()
			else:
				iParentLen = lParent.height()
			# (Not going to check the non-expanding dimension.
			# I think it would be rather puzzling for the caller to end up with
			# just a single widget on account of the parent being too "narrow".)
			iWidgets = (1 + (iParentLen - iExpandingLen) // # no space before first widget
					(iExpandingLen + iSpacing))
		assert iWidgets < RectLayout._RESERVED_CONST
		iTotalExpandingLen = iWidgets * iExpandingLen + (iWidgets - 1) * iSpacing
		if bHorizontal:
			iTotalWidth = iTotalExpandingLen
		else:
			iTotalHeight = iTotalExpandingLen
		super(_SingleFileLayout, self).__init__(lParent, fX, fY, iTotalWidth, iTotalHeight)
		assert iSpacing > -iExpandingLen, "widgets will appear out of sequence"
		self.iWidgets = iWidgets
		self.iNextWidget = 0
		self.seq = []
		# Coordinates of current widget as an offset to ours
		iBtnX = 0
		iBtnY = 0
		for i in range(self.iWidgets):
			lWidget = RectLayout(self, iBtnX, iBtnY, fWidth, fHeight)
			self.seq.append(lWidget)
			if bHorizontal:
				iBtnX += iSpacing + fWidth
			else:
				iBtnY += iSpacing + fHeight
	def numWidgets(self):
		return self.iWidgets
	def next(self):
		if self.iNextWidget >= self.iWidgets:
			return None
		lNext = self.seq[self.iNextWidget]
		self.iNextWidget += 1
		return lNext
	def resetIter(self):
		self.iNextWidget = 0
	def move(self, fX, fY):
		super(_SingleFileLayout, self).move(fX, fY)
		for lRect in self.seq:
			lRect.move(fX, fY)

class ColumnLayout(_SingleFileLayout):
	def __init__(self, lParent, fX, fY, iWidgets, iSpacing, fWidth, fHeight = None):
		super(ColumnLayout, self).__init__(
				lParent, _SingleFileLayout.VERTICAL, fX, fY, iWidgets, iSpacing, fWidth, fHeight)
class RowLayout(_SingleFileLayout):
	def __init__(self, lParent, fX, fY, iWidgets, iSpacing, fWidth, fHeight = None):
		super(RowLayout, self).__init__(
				lParent, _SingleFileLayout.HORIZONTAL, fX, fY, iWidgets, iSpacing, fWidth, fHeight)
