#!/usr/bin/env ruby

require File.join(File.dirname(File.dirname(File.expand_path(__FILE__))),
                 'external/built/share/service_testing/bp_service_runner.rb')
require 'uri'
require 'test/unit'
require 'open-uri'

def runTest_private(s, f, myself)
  json = JSON.parse(File.read(f))
  # now let's change the 'file' param to a absolute URI
  p = File.join(File.dirname(__FILE__), "test_images", json["file"])
  p = File.expand_path(p)
  # now convert p into a file url
  #json["file"] = ((p[0] == "/") ? "file://" : "file:///" ) + p
  json["file"] = "path:" + p
  took = Time.now
  r = s.transform(json)
  took = Time.now - took
  imgGot = nil
  #begin
  assert_nothing_raised {
    imgGot = File.open(r['file'], "rb") { |oi| oi.read }
    wantImgPath = File.join(File.dirname(f),
                            File.basename(f, ".json") + ".out")
    raise "no output file for test!" if !File.exist? wantImgPath
    imgWant = File.open(wantImgPath, "rb") { |oi| oi.read }
    raise "output mismatch" if imgGot != imgWant
    # yay!  it worked!
    #puts "ok. (#{r['orig_width']}x#{r['orig_height']} -> #{r['width']}x#{r['height']} took #{took}s)"
  }
  #rescue => e
  #  err = e.to_s
  #  # for convenience, if the test fails, we'll *save* the output
  #  # image in xxx.got
  #  if imgGot != nil
  #    gotPath = File.join(File.dirname(f),
  #                        File.basename(f, ".json") + ".got")
  #    File.open(gotPath, "wb") { |oi| oi.write(imgGot) }
  #    err += " [left result in #{File.basename(gotPath)}]"
  #  end
  #  puts "fail (#{err} took #{took}s)"
end

class TestFileAccess < Test::Unit::TestCase
  def setup
    # arguments are a string that must match the test name
    subdir = 'build/ImageAlter'
    if ENV.key?('BP_OUTPUT_DIR')
      subdir = ENV['BP_OUTPUT_DIR']
    end
    @cwd = File.dirname(File.expand_path(__FILE__))
    @service = File.join(@cwd, "../#{subdir}")
  end

  def teardown
  end

  def test_anim_gif_to_jpg
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "anim_gif_to_jpg.json")
      runTest_private(s, f, self)
    }
  end

  def test_black_threshold
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "black_threshold.json")
      runTest_private(s, f, self)
    }
  end

  def test_blur
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "blur.json")
      runTest_private(s, f, self)
    }
  end

  def test_contrast
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "contrast.json")
      runTest_private(s, f, self)
    }
  end

  def test_crop
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "crop.json")
      runTest_private(s, f, self)
    }
  end

  def test_despeckle
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "despeckle.json")
      runTest_private(s, f, self)
    }
  end

  def test_dither
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "dither.json")
      runTest_private(s, f, self)
    }
  end

  def test_enhance
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "enhance.json")
      runTest_private(s, f, self)
    }
  end

  def test_equalize
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "equalize.json")
      runTest_private(s, f, self)
    }
  end

  def test_ext_case_test
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "ext_case_test.json")
      runTest_private(s, f, self)
    }
  end

  def test_grayscale
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "grayscale.json")
      runTest_private(s, f, self)
    }
  end

  def test_negate
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "negate.json")
      runTest_private(s, f, self)
    }
  end

  def test_negative_rotate
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "negative_rotate.json")
      runTest_private(s, f, self)
    }
  end

  def test_noop_action_test
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "noop_action_test.json")
      runTest_private(s, f, self)
    }
  end

  def test_normalize
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "normalize.json")
      runTest_private(s, f, self)
    }
  end

  def test_oilpaint
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "oilpaint.json")
      runTest_private(s, f, self)
    }
  end

  def test_psychedelic
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "psychedelic.json")
      runTest_private(s, f, self)
    }
  end

  def test_rotate_180
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "rotate_180.json")
      runTest_private(s, f, self)
    }
  end

  def test_rotate_45
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "rotate_45.json")
      runTest_private(s, f, self)
    }
  end

  def test_rotate_900
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "rotate_900.json")
      runTest_private(s, f, self)
    }
  end

  def test_rotate_and_scale
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "rotate_and_scale.json")
      runTest_private(s, f, self)
    }
  end

  def test_rotate_and_thumbnail
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "rotate_and_thumbnail.json")
      runTest_private(s, f, self)
    }
  end

  def test_rotate_no_args
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "rotate_no_args.json")
      runTest_private(s, f, self)
    }
  end

  def test_sepia
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "sepia.json")
      runTest_private(s, f, self)
    }
  end

  def test_sharpen
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "sharpen.json")
      runTest_private(s, f, self)
    }
  end

  def test_solarize
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "solarize.json")
      runTest_private(s, f, self)
    }
  end

  def test_swirl
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "swirl.json")
      runTest_private(s, f, self)
    }
  end

  def test_threshold
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "threshold.json")
      runTest_private(s, f, self)
    }
  end

  def test_thumbnail
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "thumbnail.json")
      runTest_private(s, f, self)
    }
  end

  def test_thumbnail_and_rotate
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "thumbnail_and_rotate.json")
      runTest_private(s, f, self)
    }
  end

  def test_to_gif
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "to_gif.json")
      runTest_private(s, f, self)
    }
  end

  def test_to_png
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "to_png.json")
      runTest_private(s, f, self)
    }
  end

  def test_unsharpen
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "unsharpen.json")
      runTest_private(s, f, self)
    }
  end

  def test_washed_out_cairo
    BrowserPlus.run(@service) { |s|
      f = File.join(File.dirname(__FILE__), "cases", "washed_out_cairo.json")
      runTest_private(s, f, self)
    }
  end
end
