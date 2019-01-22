package wbo;

import java.awt.geom.Point2D;

public class Bonus {

	public Bonus(String type, int x, int y) {
		this.type = type;
		this.plot = WBOut.getOffset(x, y);
	}
	
	public String getType() {
		return type;
	}
	
	public Point2D getPlot() {
		return plot;
	}

	private String type;
	private Point2D plot;
}
