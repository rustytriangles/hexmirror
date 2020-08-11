use std::{thread, time::Duration};

use opencv::{
    core,
    highgui,
    imgproc,
    prelude::*,
    types,
    videoio,
};

fn color_at(frame: &Mat, pt: core::Point) -> core::Scalar {
    return match frame.at_2d::<core::Vec3b>(pt.y, pt.x) {
        Err(e) => {
            //println!("Err({:?})", e);
            core::Scalar::new(0., 0., 0., 0.)
        },
        Ok(p) => core::Scalar::new(p[0] as f64, p[1] as f64, p[2] as f64, 0.),
    };
}

fn make_centers(cx: i32, cy: i32, radius: i32) -> Vec<core::Point> {
    let c = (0.866 * radius as f64) as i32;
    let s = (0.5 * radius as f64) as i32;

    let mut points = Vec::new();

    let c0 = core::Point { x: cx, y: cy};
    points.push(c0);

    let c1 = core::Point { x: c0.x + c  , y: c0.y - (radius + s)};
    points.push(c1);
    points.push(core::Point { x: c0.x + 2*c, y: c0.y});
    points.push(core::Point { x: c0.x + c  , y: c0.y + (radius + s)});
    points.push(core::Point { x: c0.x - c  , y: c0.y + (radius + s)});
    points.push(core::Point { x: c0.x - 2*c, y: c0.y});
    points.push(core::Point { x: c0.x - c, y: c0.y - (radius + s)});
    
    let c7 = core::Point { x: c1.x - c, y: c1.y - (radius + s)};
    points.push(c7);
    points.push(core::Point { x: c7.x + 2*c        , y: c7.y});
    points.push(core::Point { x: points[8].x + c   , y: points[8].y + (radius + s)});
    points.push(core::Point { x: points[9].x + c   , y: points[9].y + (radius + s)}); // 10
    points.push(core::Point { x: points[10].x - c  , y: points[10].y + (radius + s)});
    points.push(core::Point { x: points[11].x - c  , y: points[11].y + (radius + s)}); // 12
    points.push(core::Point { x: points[12].x - 2*c, y: points[12].y});
    points.push(core::Point { x: points[13].x - 2*c, y: points[13].y});
    points.push(core::Point { x: points[14].x - c  , y: points[14].y - (radius + s)});
    points.push(core::Point { x: points[15].x - c  , y: points[15].y - (radius + s)});
    points.push(core::Point { x: points[16].x + c  , y: points[16].y - (radius + s)});
    points.push(core::Point { x: points[17].x + c  , y: points[17].y - (radius + s)});

    let c19 = core::Point { x: c7.x - c, y: c7.y - (radius + s)};
    points.push(c19);
    points.push(core::Point { x: points[19].x + 2*c, y: points[19].y});
    points.push(core::Point { x: points[20].x + 2*c, y: points[20].y});  // 21
    points.push(core::Point { x: points[21].x + c  , y: points[21].y + (radius + s)});
    points.push(core::Point { x: points[22].x + c  , y: points[22].y + (radius + s)});
    points.push(core::Point { x: points[23].x + c  , y: points[23].y + (radius + s)});
    points.push(core::Point { x: points[24].x - c  , y: points[24].y + (radius + s)});
    points.push(core::Point { x: points[25].x - c  , y: points[25].y + (radius + s)});
    points.push(core::Point { x: points[26].x - c  , y: points[26].y + (radius + s)}); // 27
    points.push(core::Point { x: points[27].x - 2*c, y: points[27].y});
    points.push(core::Point { x: points[28].x - 2*c, y: points[28].y});
    points.push(core::Point { x: points[29].x - 2*c, y: points[29].y});
    points.push(core::Point { x: points[30].x - c  , y: points[30].y - (radius + s)}); // 31
    points.push(core::Point { x: points[31].x - c  , y: points[31].y - (radius + s)});
    points.push(core::Point { x: points[32].x - c  , y: points[32].y - (radius + s)});
    points.push(core::Point { x: points[33].x + c  , y: points[33].y - (radius + s)});
    points.push(core::Point { x: points[34].x + c  , y: points[34].y - (radius + s)});
    points.push(core::Point { x: points[35].x + c  , y: points[35].y - (radius + s)});

    return points;
}

fn sample_at(frame: &Mat, points: &Vec<core::Point>) -> Vec<core::Scalar> {
    let black = core::Scalar::new(  0.,  0., 0., 0.);

    let mut colors = Vec::new();
    let mut first = true;
    for pt in points {
        // We don't use the color of hexagon 0 because there won't be an LED there.
        let p = if first { black } else { color_at(&frame, *pt) };
        colors.push(p);
        first = false;
    }
    return colors;
}

fn draw_hexagon(mut frame: &mut Mat, center: core::Point, radius: i32, color: core::Scalar) {
    let line_type = imgproc::LineTypes::LINE_8 as i32;
    let shift = 0;

    let c = (0.866 * radius as f64) as i32;
    let s = (0.5 * radius as f64) as i32;
    let mut pts = types::VectorOfPoint::with_capacity(1);
    pts.push(core::Point { x: center.x + c, y: center.y + s});
    pts.push(core::Point { x: center.x, y: center.y + radius});
    pts.push(core::Point { x: center.x - c, y: center.y + s});
    pts.push(core::Point { x: center.x - c, y: center.y - s});
    pts.push(core::Point { x: center.x, y: center.y - radius});
    pts.push(core::Point { x: center.x + c, y: center.y - s});

    imgproc::fill_convex_poly(&mut frame, &pts, color, line_type, shift);
}

fn xflip(points: &Vec<core::Point>, x: i32) -> Vec<core::Point> {
    let mut result = Vec::new();
    for pt in points {
        result.push(core::Point { x: x - pt.x, y: pt.y });
    }
    return result;
}

fn run() -> opencv::Result<()> {

    // Create a window
    let window = "video capture";
    highgui::named_window(window, 1)?;

    // Open the video camera
    let mut cam = videoio::VideoCapture::new(0, videoio::CAP_ANY)?;  // 0 is the default camera
    if !videoio::VideoCapture::is_opened(&cam)? {
        panic!("Unable to open default camera!");
    }

    loop {
        let mut frame = Mat::default()?;

        cam.read(&mut frame)?;
        if frame.size()?.width == 0 {
            thread::sleep(Duration::from_secs(50));
            continue;
        }

        let w = frame.size()?.width;
        let h = frame.size()?.height;
        let radius = 64;

        // generate the centers of the hexagons
        let points = make_centers(w/2, h/2, radius);

        // sample the pixel colors at those locations
        let colors = sample_at(&frame, &points);

        let line_width = imgproc::LineTypes::FILLED as i32;
        let line_type = imgproc::LineTypes::LINE_8 as i32;
        let shift = 0;

        // clear everything to grey
        let grey   = core::Scalar::new( 192., 192., 192., 0.);
        imgproc::rectangle(&mut frame,
            core::Rect {x: 0, y: 0, width: w, height: h },
            grey,
            line_width, line_type, shift)?;

        let mirror = true;
        let mirror_points = xflip(&points, w);

        // draw the hexagons in the sampled colors
        for i in 0..37 {
            let pt = if mirror { mirror_points[i] } else { points[i] };
            draw_hexagon(&mut frame, pt, radius, colors[i]);
        }

        // Display the image
        highgui::imshow(window, &frame)?;

        // Wait 10 milliseconds and quit if a key was pressed
        if highgui::wait_key(10)? > 0 {
            break;
        }
    }
    Ok(())
}

fn main() {
    run().unwrap()
}
