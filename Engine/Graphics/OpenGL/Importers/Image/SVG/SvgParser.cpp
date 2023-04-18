#include "SvgParser.h"
/*struct PathCommand {
	char type;
	std::vector<float> values;
};

struct Path {
	std::vector<PathCommand> commands;
};

struct SVG {
	int width;
	int height;
	std::vector<Path> paths;
};

SVG parseSVG(const char* filename) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Error: Could not open file " << filename << std::endl;
		return {};
	}

	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();

	SVG svg = {};

	std::regex width_regex(R"(width=\"([\d.]+)(\w+)\")");
		std::regex height_regex(R"(height=\"([\d.]+)(\w+)\")");
		std::smatch match;

	float svg_width = 0;
	float svg_height = 0;

	if (std::regex_search(content, match, width_regex)) {
		svg_width = std::stof(match[1]);
		std::string unit = match[2];
		if (unit == "mm") svg_width *= 3.7795275591f;
		else if (unit == "cm") svg_width *= 37.795275591f;
		else if (unit == "in") svg_width *= 96;
		else if (unit == "pt") svg_width *= 1.3333333333f;
		else if (unit == "pc") svg_width *= 16;
	}

	if (std::regex_search(content, match, height_regex)) {
		svg_height = std::stof(match[1]);
		std::string unit = match[2];
		if (unit == "mm") svg_height *= 3.7795275591f;
		else if (unit == "cm") svg_height *= 37.795275591f;
		else if (unit == "in") svg_height *= 96;
		else if (unit == "pt") svg_height *= 1.3333333333f;
		else if (unit == "pc") svg_height *= 16;
	}

	svg.width = (int)svg_width;
	svg.height = (int)svg_height;

	std::regex path_regex(R"(<path[^>]*d=\"([^"]+)\")");
		auto path_begin = std::sregex_iterator(content.begin(), content.end(), path_regex);
	auto path_end = std::sregex_iterator();

	for (std::sregex_iterator i = path_begin; i != path_end; ++i) {
		Path path = {};
		std::string d = (*i)[1];
		std::regex command_regex(R"(([MmLlHhVvCcSsQqTtAaZz])|(-?[\d.]+))");
		auto command_begin = std::sregex_iterator(d.begin(), d.end(), command_regex);
		auto command_end = std::sregex_iterator();
		PathCommand command = {};
		for (std::sregex_iterator j = command_begin; j != command_end; ++j) {
			std::string token = (*j)[0];
			if (std::isalpha(token[0])) {
				if (!command.values.empty()) {
					path.commands.push_back(command);
					command.values.clear();
				}
				command.type = token[0];
			}
			else {
				command.values.push_back(std::stof(token));
			}
		}
		if (!command.values.empty()) {
			path.commands.push_back(command);
			command.values.clear();
		}
		svg.paths.push_back(path);
	}

	return svg;
}
float svgAngle(float ux, float uy, float vx, float vy)
{
	float dot = ux * vx + uy * vy;
	float len = sqrt(ux * ux + uy * uy) * sqrt(vx * vx + vy * vy);
	float angle = acos(dot / len);
	if ((ux * vy - uy * vx) < 0.0f)
		angle = -angle;
	return angle;
}

void drawLine(unsigned char* data, int width, int height, int x0, int y0, int x1, int y1, int desiredChannels) {
	int dx = abs(x1 - x0);
	int dy = abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx - dy;

	while (true) {
		if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < height) {
			int index = (y0 * width + x0) * desiredChannels;
			data[index] = 255;
			data[index + 1] = 255;
			data[index + 2] = 255;
			if (desiredChannels == 4) data[index + 3] = 255;
		}

		if (x0 == x1 && y0 == y1) break;

		int e2 = err * 2;
		if (e2 > -dy) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dx) {
			err += dx;
			y0 += sy;
		}
	}
}

void drawCubicBezier(unsigned char* data, int width, int height, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, int desiredChannels)
{
	const float steps = 100.0f;
	for (float t = 0.0f; t <= 1.0f; t += 1.0f / steps)
	{
		float u = 1.0f - t;
		float tt = t * t;
		float uu = u * u;
		float uuu = uu * u;
		float ttt = tt * t;

		float x = (uuu * x0) + (3 * uu * t * x1) + (3 * u * tt * x2) + (ttt * x3);
		float y = (uuu * y0) + (3 * uu * t * y1) + (3 * u * tt * y2) + (ttt * y3);

		drawPixel(data, width, height, round(x), round(y));
	}
}
void getArcCenter(float x1, float y1, float rx, float ry, float angle, float largeArcFlag, float sweepFlag, float x2, float y2, float* cx, float* cy)
{
	// Convert angle from degrees to radians
	angle = angle * M_PI / 180.0f;

	// Compute the half distance between the current and the final point
	float dx2 = (x1 - x2) / 2.0f;
	float dy2 = (y1 - y2) / 2.0f;

	// Compute the transformed half distance
	float x1p = cos(angle) * dx2 + sin(angle) * dy2;
	float y1p = -sin(angle) * dx2 + cos(angle) * dy2;

	// Ensure radii are large enough
	rx = fabs(rx);
	ry = fabs(ry);
	float Prx = rx * rx;
	float Pry = ry * ry;
	float Px1 = x1p * x1p;
	float Py1 = y1p * y1p;
	// check if radii are large enough
	float radiiCheck = Px1 / Prx + Py1 / Pry;
	if (radiiCheck > 1.0f)
	{
		rx = sqrt(radiiCheck) * rx;
		ry = sqrt(radiiCheck) * ry;
		Prx = rx * rx;
		Pry = ry * ry;
	}

	// Compute the transformed center
	float sign = (largeArcFlag == sweepFlag) ? -1.0f : 1.0f;
	float sq = ((Prx * Pry) - (Prx * Py1) - (Pry * Px1)) / ((Prx * Py1) + (Pry * Px1));
	sq = (sq < 0.0f) ? 0.0f : sq;
	float coef = sign * sqrt(sq);
	float cxp = coef * ((rx * y1p) / ry);
	float cyp = coef * -((ry * x1p) / rx);

	// Compute the center in the original coordinate system
	*cx = cos(angle) * cxp - sin(angle) * cyp + ((x1 + x2) / 2.0f);
	*cy = sin(angle) * cxp + cos(angle) * cyp + ((y1 + y2) / 2.0f);
}

void drawPixel(unsigned char* data, int width, int height, int x, int y)
{
	if (x >= 0 && x < width && y >= 0 && y < height)
	{
		int index = (y * width + x) * 4;
		data[index] = 0;
		data[index + 1] = 0;
		data[index + 2] = 0;
		data[index + 3] = 255;
	}
}
void drawArc(unsigned char* data, int width, int height, float cx, float cy, float rx, float ry, float startAngle, float endAngle, bool isCounterClockwise, int desiredChannels)
{
	if (rx <= 0.0f || ry <= 0.0f)
		return;

	// Compute the angular distance between the start and end points
	float angularDistance = endAngle - startAngle;
	if (isCounterClockwise)
	{
		if (angularDistance <= 0.0f)
			angularDistance += 2.0f * M_PI;
	}
	else
	{
		if (angularDistance >= 0.0f)
			angularDistance -= 2.0f * M_PI;
	}

	// Compute the number of segments to approximate the arc
	int segmentCount = ceil(fabs(angularDistance) / (M_PI / 4.0f));
	float angleIncrement = angularDistance / segmentCount;

	// Draw each segment
	float x1 = cx + rx * cos(startAngle);
	float y1 = cy + ry * sin(startAngle);
	for (int i = 0; i < segmentCount; i++)
	{
		float angle2 = startAngle + (i + 1) * angleIncrement;
		float x2 = cx + rx * cos(angle2);
		float y2 = cy + ry * sin(angle2);
		drawLine(data, width, height, round(x1), round(y1), round(x2), round(y2), desiredChannels);
		x1 = x2;
		y1 = y2;
	}
}
void rasterizeSVG(const SVG& svg, unsigned char* data, int desiredChannels)
{
	for (const Path& path : svg.paths)
	{
		float x = 0;
		float y = 0;
		for (const PathCommand& command : path.commands)
		{
			const std::vector<float>& values = command.values;
			switch (command.type)
			{
			case 'M':
				x = values[0];
				y = values[1];
				break;
			case 'L':
				drawLine(data, svg.width, svg.height, x, y, values[0], values[1], desiredChannels);
				x = values[0];
				y = values[1];
				break;
			case 'C':
				drawCubicBezier(data, svg.width, svg.height, x, y, values[0], values[1], values[2], values[3], values[4], values[5], desiredChannels);
				x = values[4];
				y = values[5];
				break;
			case 'A':
			{
				float rx = values[0];
				float ry = values[1];
				float angle = values[2];
				float largeArcFlag = values[3];
				float sweepFlag = values[4];
				float x2 = values[5];
				float y2 = values[6];
				float cx;
				float cy;
				getArcCenter(x, y, rx, ry, angle, largeArcFlag, sweepFlag, x2, y2, &cx, &cy);
				float startAngle = svgAngle(1.0f, 0.0f, (x - cx) / rx, (y - cy) / ry);
				float endAngle = svgAngle(1.0f, 0.0f, (x2 - cx) / rx, (y2 - cy) / ry);
				drawArc(data, svg.width, svg.height, cx, cy, rx, ry, startAngle, endAngle,
					sweepFlag == 0.0f, desiredChannels);
				x = x2;
				y = y2;
			}
			break;
			}
		}
	}
}
unsigned char* LoadSVG(const char* filename, int* width, int* height, int* bpp, int desiredChannels)
{
	SVG svg = parseSVG(filename);
	*width = svg.width;
	*height = svg.height;
	*bpp = desiredChannels;
	unsigned char* data = new unsigned char[(*width) * (*height) * desiredChannels];
	rasterizeSVG(svg, data, desiredChannels);
	return data;
}*/
bool invertes = true;

unsigned char* LoadSVG(const char* filename, int* width, int* height, int* bpp, int desiredChannels) {
	// Parse the SVG file using NanoSVG
	NSVGimage* image = nsvgParseFromFile(filename, "px", 96.0f);
	if (!image)
	{
		std::cerr << "Error: Could not parse SVG file " << filename << std::endl;
		return nullptr;
	}

	// Set the output parameters
	*width = (int)image->width;
	*height = (int)image->height;
	*bpp = desiredChannels;

	// Create a rasterizer and rasterize the image
	NSVGrasterizer* rast = nsvgCreateRasterizer();
	unsigned char* data = new unsigned char[(*width) * (*height) * desiredChannels];
	nsvgRasterize(rast, image, 0, 0, 1, data, *width, *height, (*width) * desiredChannels);

	// Handle vertical inversion if necessary
	if (invertes)
	{
		for (int y = 0; y < (*height) / 2; y++)
		{
			for (int x = 0; x < (*width); x++)
			{
				for (int c = 0; c < desiredChannels; c++)
				{
					int i1 = (y * (*width) + x) * desiredChannels + c;
					int i2 = (((*height) - y - 1) * (*width) + x) * desiredChannels + c;
					std::swap(data[i1], data[i2]);
				}
			}
		}
	}

	// Clean up and return the rasterized image data
	nsvgDeleteRasterizer(rast);
	nsvgDelete(image);
	return data;
}