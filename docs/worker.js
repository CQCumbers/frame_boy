/**
 * Welcome to your Workbox-powered service worker!
 *
 * You'll need to register this file in your web app and you should
 * disable HTTP caching for this file too.
 * See https://goo.gl/nhQhGp
 *
 * The rest of the code is auto-generated. Please don't update this file
 * directly; instead, make changes to your Workbox build configuration
 * and re-run your build process.
 * See https://goo.gl/2aRDsh
 */

importScripts("https://storage.googleapis.com/workbox-cdn/releases/4.3.1/workbox-sw.js");

self.addEventListener('message', (event) => {
  if (event.data && event.data.type === 'SKIP_WAITING') {
    self.skipWaiting();
  }
});

/**
 * The workboxSW.precacheAndRoute() method efficiently caches and responds to
 * requests for URLs in the manifest.
 * See https://goo.gl/S9QRab
 */
self.__precacheManifest = [
  {
    "url": "assets/android-chrome-192x192.png",
    "revision": "a7501bad6c2dc757572cf4b8335f66b5"
  },
  {
    "url": "assets/android-chrome-512x512.png",
    "revision": "b729243a0de4a21fc9d330e2d0a84317"
  },
  {
    "url": "assets/apple-touch-icon.png",
    "revision": "b9c4bedbe4b0a8eb5177e61b11a1845d"
  },
  {
    "url": "assets/favicon-16x16.png",
    "revision": "0f92bc117f684177a608d245f77fecf9"
  },
  {
    "url": "assets/favicon-32x32.png",
    "revision": "8b27bf316aa6c74715630ab3fdb29716"
  },
  {
    "url": "favicon.ico",
    "revision": "d8948cbccd0e4c4d78ca0e2e004a71de"
  },
  {
    "url": "index.html",
    "revision": "77c58fc11feaf2a22406eab4bfd1980b"
  },
  {
    "url": "index.js",
    "revision": "a21ec98524e8b7ca98823ca206935d33"
  },
  {
    "url": "index.wasm",
    "revision": "ba468ff9555bfa0f294d29c5233c883c"
  },
  {
    "url": "site.webmanifest",
    "revision": "a3db5c19fc706a315d5d226bcfe152c4"
  },
  {
    "url": "style.css",
    "revision": "b494038b9402848834467de6b4550082"
  }
].concat(self.__precacheManifest || []);
workbox.precaching.precacheAndRoute(self.__precacheManifest, {});
