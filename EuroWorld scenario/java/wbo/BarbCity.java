package wbo;

import java.awt.Point;
import java.awt.geom.Point2D;
import java.util.ArrayList;
import java.util.Arrays;

public class BarbCity {

	public BarbCity(String name, int x, int y) {
		this.name = name;
		init(x, y);
	}

	public BarbCity(String name, int x, int y, String[] religions) {
		this.name = name;
		init(x, y);
		this.religions.addAll(Arrays.asList(religions));
	}
	
	public String getName() {
		return name;
	}
	
	public Point2D getPlot() {
		return plot;
	}
	
	public ArrayList<String> getReligions() {
		return religions;
	}
	
	private void init(int x, int y) {
		this.plot = new Point(x + WBOut.offsetX, y +  WBOut.offsetY);
		religions = new ArrayList<String>();
	}

	private String name;
	private Point2D plot;
	private ArrayList<String> religions;
}
