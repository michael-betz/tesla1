$fn=75;

r_plate = 163 / 2;
h_plate = 4.6 + 0.2;
T = 5;   // plastic thickness / primary height
A = 25;  // slot depth
cone_offset = 3;
h_holder = 25;
d_wire = 3.1;
N_holders =6;

d_mounting_hole = 3.6;

module chamfer_block() {
	translate([5 / sqrt(2), 5 / sqrt(2)]) rotate([0, 0, 45]) square([10, 20], center=true);
}

module 2d_shape() {
	difference() {
		polygon([
			[0, 0],
			[0, -T],
			[A + T, -T],
			[A + T, T + h_plate],
			[T + cone_offset, T + h_plate],
			[T + cone_offset, T + h_plate + h_holder],
			[cone_offset, T + h_plate + h_holder],
			[0, h_plate],
			[A, h_plate],
			[A, 0]
		]);
		translate([T + cone_offset, T + h_plate, 0])
			for (i = [d_wire / 2: d_wire: h_holder]) {
				translate([0, i, 0]) circle(r=d_wire / 2);
			}
		chamfer_val = 1.5;
		translate([A + T - chamfer_val, T + h_plate - chamfer_val])
			chamfer_block();
		translate([A + T - chamfer_val, -T + chamfer_val])
			rotate([180, 0]) chamfer_block();
	}
}

module 3d_shape() {
	// radius and angle of mounting hole
	r_temp = r_plate - d_mounting_hole * 1.5;
	alpha = 45 / 4 / 2;
	difference() {
		rotate_extrude(angle=45 / 4, $fn=200)
			translate([r_plate - A, 0, 0]) 2d_shape();
		// Drill a mounting hole
		translate([r_temp * cos(alpha), r_temp * sin(alpha), -6])
			cylinder(h=10, r=d_mounting_hole / 2, center=false, $fn=20);
	}
}

// 2d_shape();

// ---------------------------------
//  Model of how it will look like
// ---------------------------------
// cylinder(h=h_plate, r=r_plate);
// cylinder(h=300, r=110 / 2);
// color("grey")
// 	for (i = [0: 360 / N_holders: 359]) {
// 		echo(i);
// 		rotate([0, 0, i]) 3d_shape();
// 	}

// ----------------
//  for exporting
// ----------------
3d_shape();
