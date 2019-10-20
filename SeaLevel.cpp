#include "stdafx.h"
#include "aquaring.h"
#include "math.h"
#include "stdio.h"
//#include "PxPhysicsAPI.h"

const float EarthsRadius = 6.371E6;

typedef struct _ALTIMETRY_DATA {
	// Inputs
	float Latitude;
	float Longtitude;
	float LatitudeStep;
	float LongtitudeStep;
	// Outputs
	float GroundElevation;
	float WaterSurfaceElevation;
	float IceSurfaceElevation;
	float SnowSurfaceElevation;
	float RegionAreaAtZeroElevation;
	int WaterType;
} ALTIMETRY_DATA;

typedef struct _HIST {
	float TotalVolume;
	float WaterMass;
	float WaterVolume;
} HIST;

enum { SeaWater, FreshWater };

float GetWaterDensity(float Depth, int WaterType) {

	if (WaterType == SeaWater) {
		return 1027; // kg/m3
		// With this change significantly as the icecaps melt?
	}
	else if (WaterType == FreshWater) {
		return 1000; // kg/m3
	}
	else {
		// Add Ice???
		return 1000;
	}
}

float InvGamma(float colcomp) {
	return powf(colcomp / 255.0f, 2.2f) * 255.0f;
}

void GetAltimetry(ALTIMETRY_DATA &a, Gdiplus::Bitmap *bmpelev, Gdiplus::Bitmap *bmpbath) {

	int x, y, w, h;
	Gdiplus::Color Col;
	float r;

	w = bmpbath->GetWidth();
	h = bmpbath->GetHeight();
	x = (int)((a.Longtitude / 360.0f) * w);
	y = (int)((0.5f + a.Latitude / 180.0f) * h);
	bmpbath->GetPixel(x, y, &Col);
	r = Col.GetRed();

	if (r < 255.0f) {
		a.GroundElevation = -11000.0f * InvGamma(r) / 255.0f;
		a.WaterSurfaceElevation = 0.0f;
		a.WaterType = SeaWater;
	}
	else {
		w = bmpelev->GetWidth();
		h = bmpelev->GetHeight();
		x = (int)((a.Longtitude / 360.0f) * w);
		y = (int)((0.5f + a.Latitude / 180.0f) * h);
		bmpelev->GetPixel(x, y, &Col);
		r = Col.GetRed();
		a.GroundElevation = 11000.0f * InvGamma(r) / 255.0f;
		a.WaterSurfaceElevation = a.GroundElevation;
		a.WaterType = FreshWater;
	}
	a.IceSurfaceElevation = 0.0f;
	a.SnowSurfaceElevation = 0.0f;
	a.RegionAreaAtZeroElevation = EarthsRadius * a.LongtitudeStep * EarthsRadius * a.LatitudeStep * (float)cos(a.Latitude * PI / 180.0f);

}

DWORD WINAPI SeaLevel(LPVOID Arguments) {

	// Thread Management...
	PSeaLevelArguments args;
	args = (PSeaLevelArguments)Arguments;

	HDC hdc = args->hdc;
	SIM_PARAM *sp = args->sp;
	// End Thread Management.

	float ScreenCenterX;
	float ScreenCenterY;
	float SceneWidth;
	float SceneHeight;
	int year;
	float Tension;
	float lat;
	float lon;
	float LatitudeStep;
	float LongtitudeStep;
	ALTIMETRY_DATA a;
	float ElevationStep;
	int el;
	int elbin;
	int top_el_bin;
	int bot_el_bin;
	float PartialFactor;
	float WaterColumnVolume;
	float Depth;

	const int numpt = 2200;
	HIST Histogram[numpt];
	Gdiplus::PointF pt[numpt];
	Gdiplus::PointF lpt[2];

	sp->kp.ScaleFactor = 1.0;

	ScreenCenterX = 50; // (float)(sp->ClientRect.right + sp->ClientRect.left) / 2;
	SceneWidth = (float)ceil((sp->ClientRect.right - sp->ClientRect.left) / sp->kp.ScaleFactor);
	ScreenCenterY = (float)(sp->ClientRect.bottom + sp->ClientRect.top) / 2;
	SceneHeight = (float)ceil((sp->ClientRect.bottom - sp->ClientRect.top) / sp->kp.ScaleFactor);

	// Initialize Graphics
	Gdiplus::Graphics gf(hdc);
	//Gdiplus::Pen pen(Gdiplus::Color(255, 15, 19, 80));               // For lines, rectangles and curves
	Gdiplus::Pen pen(Gdiplus::Color(255, 255, 19, 80));
	Gdiplus::Pen graypen(Gdiplus::Color(255, 80, 80, 80));
	Gdiplus::SolidBrush brush(Gdiplus::Color(255, 15, 19, 80));      // For filled shapes
	gf.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	Tension = 0.35f;

	gf.Clear(Gdiplus::Color::White);
	Gdiplus::Bitmap bmpelev(L"gebco_08_rev_elev_21600x10800_2.png");
	Gdiplus::Bitmap bmpbath(L"gebco_08_rev_bath_21600x10800.png");

	float ScaleFactor;
	ScaleFactor = 0.06f;
	Gdiplus::GraphicsContainer container;
	container = gf.BeginContainer();
	gf.ScaleTransform(ScaleFactor, ScaleFactor, Gdiplus::MatrixOrderPrepend);
	//gf.DrawImage(&bmpelev, 0, 0, bmpelev.GetWidth(), bmpelev.GetHeight());
	gf.DrawImage(&bmpelev, 0, 0, bmpbath.GetWidth(), bmpbath.GetHeight());
	gf.EndContainer(container);

	/*
	int x, y;
	Gdiplus::Color Col;
	for (y = 0; y < 10800; y++) {
		for (x = 0; x < 21600; x++) {
			bmpbath.GetPixel(x, y, &Col);
			float r = Col.GetRed();
			float g = Col.GetGreen();
			float b = Col.GetBlue();
			if (r > 100) {
				__asm {nop}
			}
			float alpha = Col.GetAlpha();
		}
	}
	*/

	ElevationStep = 12.0f; // Meter
	LatitudeStep = 1.0f; // Degrees
	LongtitudeStep = 1.0f; // Degrees

	a.LatitudeStep = LatitudeStep;
	a.LongtitudeStep = LongtitudeStep;
	for (year = 1900; year <= 1900; year++) {
		// Determine the distribution of water of the planet

		for (elbin = 0; elbin < numpt; elbin++) {
			Histogram[elbin].TotalVolume = 0.0f;
			Histogram[elbin].WaterVolume = 0.0f;
			Histogram[elbin].WaterMass = 0.0f;
		}

		for (lat = -90 + LatitudeStep/2; lat <= 90 - LatitudeStep/2; lat += LatitudeStep) {
			for (lon = LongtitudeStep/2; lon < 360 - LongtitudeStep/2; lon += LongtitudeStep) {
				a.Latitude = lat;
				a.Longtitude = lon;
				GetAltimetry(a, &bmpelev, &bmpbath);
				elbin = numpt/2; // This is the bit the represents elevations between -ElevationStep and Zero.
				top_el_bin = numpt - 1;
				bot_el_bin = (int)floor(a.GroundElevation / ElevationStep);

				for (el = bot_el_bin; el <= top_el_bin; el++) {
					if (el == bot_el_bin) PartialFactor = (a.GroundElevation - bot_el_bin * ElevationStep) / ElevationStep;
					else PartialFactor = 1.0;
					elbin = max(0, min(numpt - 1, numpt / 2 + el));
					WaterColumnVolume = a.RegionAreaAtZeroElevation * (EarthsRadius + el) / EarthsRadius * ElevationStep * PartialFactor;
					Histogram[elbin].TotalVolume += WaterColumnVolume;
				}

				top_el_bin = (int)ceil(a.WaterSurfaceElevation / ElevationStep);

				if (0) for (el = bot_el_bin; el <= top_el_bin; el++) {
					if ((el == bot_el_bin) && (el == top_el_bin)) PartialFactor = (a.WaterSurfaceElevation - a.GroundElevation) / ElevationStep;
					else if (el == bot_el_bin) PartialFactor = (a.GroundElevation - bot_el_bin * ElevationStep) / ElevationStep;
					else if (el == top_el_bin) PartialFactor = (top_el_bin * ElevationStep - a.WaterSurfaceElevation) / ElevationStep;
					else PartialFactor = 1.0;
					elbin = max(0, min(numpt-1, numpt / 2 + el));
					WaterColumnVolume = a.RegionAreaAtZeroElevation * (EarthsRadius + el) / EarthsRadius * ElevationStep * PartialFactor;
					Histogram[elbin].WaterVolume += WaterColumnVolume;
					Depth = a.WaterSurfaceElevation - el * ElevationStep;
					Histogram[elbin].WaterMass += WaterColumnVolume * GetWaterDensity(Depth, a.WaterType);
				}
			}
		}

		float ScaledWaterVolume;
		for (elbin = 0; elbin < numpt; elbin++) {
			ScaledWaterVolume = Histogram[elbin].TotalVolume * 5.5e-17f;
			if (1) {
				pt[elbin].X = ScreenCenterX + ScaledWaterVolume;
			}
			else {
				if (ScaledWaterVolume <= 1.0f) {
					pt[elbin].X = ScreenCenterX;
				}
				else {
					pt[elbin].X = ScreenCenterX + logf(ScaledWaterVolume) * 100;
				}
			}
			if (1) {
				el = elbin - numpt / 2;
				pt[elbin].Y = ScreenCenterY - (float)el / (numpt / 2) * SceneHeight / 2;
			}
			else {
				el = elbin - numpt / 2;
				pt[elbin].Y = ScreenCenterY - sqrtf(fabsf(((float)el / (numpt / 2)))) * copysignf(1.0f, (float)el) * SceneHeight / 2;
			}
		}
		gf.DrawCurve(&pen, pt, numpt - 1, Tension);
		lpt[0].X = ScreenCenterX;
		lpt[0].Y = ScreenCenterY;
		lpt[1].X = ScreenCenterX + SceneWidth;
		lpt[1].Y = ScreenCenterY;
		gf.DrawLine(&graypen, lpt[0], lpt[1]);

	}

	return 0;

}

