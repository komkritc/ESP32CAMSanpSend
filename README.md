# SnapSend: ESP32-CAM Image Upload to Django Server

![SnapSend](https://github.com/komkritc/ESP32CAMSanpSend/blob/main/images/snapsend.png)

SnapSend is a project that enables the ESP32-CAM module to capture images and send them to a Django-based server. This repository contains the code and instructions for setting up the ESP32-CAM and Django server for seamless image transmission.

## Table of Contents
- [Overview](#overview)
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [Setup Instructions](#setup-instructions)
  - [ESP32-CAM Setup](#esp32-cam-setup)
  - [Django Server Setup](#django-server-setup)
- [Usage](#usage)

---

## Overview
SnapSend leverages the ESP32-CAM's capabilities to capture images and send them to a Django server via HTTP POST requests. The Django server processes and stores these images, allowing for further use in web applications or data analysis.

## Features
- Capture high-quality images using the ESP32-CAM module.
- Send images to a Django server using HTTP POST requests.
- Store and manage uploaded images on the server.
- Simple configuration and setup.

## Hardware Requirements
- **ESP32-CAM** module with OV2640 camera.
- FTDI programmer for flashing the ESP32-CAM.
- 5V power supply for the ESP32-CAM.
- A computer with USB ports for development.

## Software Requirements
- **Arduino IDE** with ESP32 board support installed.
- Python 3.x with Django installed.
- Required Python libraries: `pillow` (for image handling), `django`.

---

## Setup Instructions

### ESP32-CAM Setup
1. **Install Arduino IDE**:
   - Install the Arduino IDE and add the ESP32 board support via the Board Manager.

2. **Connect ESP32-CAM**:
   - Connect the ESP32-CAM to your computer using an FTDI programmer. Ensure proper wiring (e.g., GPIO0 to GND for flashing).

3. **Upload Code**:
   - Use the provided Arduino sketch (`esp32_cam_post.ino`) in this repository.
   - Configure Wi-Fi credentials and server URL in the sketch:
     ```
     const char* ssid = "YOUR_WIFI_SSID";
     const char* password = "YOUR_WIFI_PASSWORD";
     const char* serverName = "http://your-server-ip/upload/";
     ```
   - Compile and upload the code to the ESP32-CAM.

4. **Test Camera**:
   - Ensure that the camera initializes correctly by checking serial monitor logs.

### Django Server Setup
1. **Install Django**:
   - Install Django using pip: `pip install django pillow`.

2. **Create Project**:
   - Create a new Django project and app:
     ```
     django-admin startproject snapsend_server
     cd snapsend_server
     python manage.py startapp uploads
     ```

3. **Configure Settings**:
   - Add `uploads` to `INSTALLED_APPS` in `settings.py`.
   - Configure media file handling in `settings.py`:
     ```
     MEDIA_URL = '/media/'
     MEDIA_ROOT = BASE_DIR / 'media'
     ```

4. **Define Model**:
   - In `uploads/models.py`, define an ImageField model:
     ```
     from django.db import models

     class UploadedImage(models.Model):
         image = models.ImageField(upload_to='images/')
         timestamp = models.DateTimeField(auto_now_add=True)
     ```

5. **Create View**:
   - In `uploads/views.py`, create a view to handle image uploads:
     ```
     from django.http import JsonResponse
     from .models import UploadedImage

     def upload_image(request):
         if request.method == 'POST' and request.FILES['image']:
             uploaded_image = UploadedImage(image=request.FILES['image'])
             uploaded_image.save()
             return JsonResponse({'message': 'Image uploaded successfully!'})
         return JsonResponse({'error': 'Invalid request'}, status=400)
     ```

6. **Configure URLs**:
   - Add URL patterns in `snapsend_server/urls.py`:
     ```
     from django.conf import settings
     from django.conf.urls.static import static
     from django.urls import path
     from uploads.views import upload_image

     urlpatterns = [
         path('upload/', upload_image, name='upload_image'),
     ] + static(settings.MEDIA_URL, document_root=settings.MEDIA_ROOT)
     ```

7. **Run Server**:
   - Start the Django development server: `python manage.py runserver`.

---
![SnapSend](https://github.com/komkritc/ESP32CAMSanpSend/blob/main/images/screenapi.png)
## Usage
1. Power on your ESP32-CAM module.
2. The module will connect to Wi-Fi and start sending images to the Django server.
3. Access uploaded images on the server in the `/media/uploads/` directory or through a web interface if implemented.

---
