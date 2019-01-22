package wbo;

import java.awt.geom.Point2D;
import static wbo.Direction.*;


public class River {
	
	public River(Direction of, int x, int y, Direction to) {
		assert(of != N && of != W);
		assert(of == S || to == S || to == N);
		assert(of == E || to == E || to == W);
		this.of = of;
		this.plot = WBOut.getOffset(x, y);
		this.to = to;
	}
	
	public Direction getOf() {
		return of;
	}
	
	public Direction getTo() {
		return to;
	}
	
	public Point2D getPlot() {
		return plot;
	}

	private Direction of, to;
	private Point2D plot;
}
