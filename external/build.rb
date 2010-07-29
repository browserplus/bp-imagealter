#!/usr/bin/env ruby

$cwd = File.dirname(__FILE__)
require File.join($cwd, "bakery/ports/bakery")

$order = {
  :output_dir => File.join($cwd, "dist"),
  :packages => [
                "zlib",
                "libpng",
                "jpeg",
                "GraphicsMagick",
                "service_testing"
               ],
  :verbose => true
}

b = Bakery.new $order
b.build
