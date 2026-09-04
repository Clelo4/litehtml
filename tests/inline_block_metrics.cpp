#include <litehtml.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace
{
    class metrics_test_container final : public litehtml::document_container
    {
      public:
        litehtml::uint_ptr create_font(const litehtml::font_description& descr, const litehtml::document*,
                                       litehtml::font_metrics* metrics) override
        {
            if(metrics)
            {
                metrics->font_size   = descr.size;
                metrics->height      = descr.size;
                metrics->ascent      = descr.size * 3 / 4;
                metrics->descent     = descr.size - metrics->ascent;
                metrics->x_height    = descr.size / 2;
                metrics->ch_width    = descr.size / 2;
                metrics->sub_shift   = descr.size / 3;
                metrics->super_shift = descr.size / 3;
            }
            return 1;
        }

        void delete_font(litehtml::uint_ptr) override {}

        litehtml::pixel_t text_width(const char* text, litehtml::uint_ptr) override
        {
            return static_cast<int>(std::strlen(text)) * 8;
        }

        void draw_text(litehtml::uint_ptr, const char*, litehtml::uint_ptr, litehtml::web_color,
                       const litehtml::position&) override
        {
        }

        litehtml::pixel_t pt_to_px(float pt) const override { return pt * 96 / 72; }
        litehtml::pixel_t get_default_font_size() const override { return 16; }
        const char*       get_default_font_name() const override { return "sans"; }
        void              draw_list_marker(litehtml::uint_ptr, const litehtml::list_marker&) override {}
        void              load_image(const char*, const char*, bool) override {}
        void              get_image_size(const char*, const char*, litehtml::size& size) override { size = {0, 0}; }
        void draw_image(litehtml::uint_ptr, const litehtml::background_layer&, const std::string&,
                        const std::string&) override
        {
        }
        void draw_solid_fill(litehtml::uint_ptr, const litehtml::background_layer&, const litehtml::web_color&) override {}
        void draw_linear_gradient(litehtml::uint_ptr, const litehtml::background_layer&,
                                  const litehtml::background_layer::linear_gradient&) override
        {
        }
        void draw_radial_gradient(litehtml::uint_ptr, const litehtml::background_layer&,
                                  const litehtml::background_layer::radial_gradient&) override
        {
        }
        void draw_conic_gradient(litehtml::uint_ptr, const litehtml::background_layer&,
                                 const litehtml::background_layer::conic_gradient&) override
        {
        }
        void draw_borders(litehtml::uint_ptr, const litehtml::borders&, const litehtml::position&, bool) override {}
        void set_caption(const char*) override {}
        void set_base_url(const char*) override {}
        void link(const std::shared_ptr<litehtml::document>&, const litehtml::element::ptr&) override {}
        void on_anchor_click(const char*, const litehtml::element::ptr&) override {}
        void on_mouse_event(const litehtml::element::ptr&, litehtml::mouse_event) override {}
        void set_cursor(const char*) override {}
        void transform_text(std::string&, litehtml::text_transform) override {}
        void import_css(std::string&, const std::string&, std::string&) override {}
        void set_clip(const litehtml::position&, const litehtml::border_radiuses&) override {}
        void del_clip() override {}
        void get_viewport(litehtml::position& viewport) const override { viewport = {0, 0, 320, 480}; }
        litehtml::element::ptr create_element(const char*, const litehtml::string_map&,
                                              const std::shared_ptr<litehtml::document>&) override
        {
            return nullptr;
        }
        void get_media_features(litehtml::media_features& media) const override
        {
            media.type       = litehtml::media_type_screen;
            media.width      = 320;
            media.height     = 480;
            media.resolution = 96;
        }
        void get_language(std::string&, std::string&) const override {}
    };

    bool nested_inline_block_reserves_its_resolved_width()
    {
        metrics_test_container container;
        const char* html = R"(
            <style>
                #outer, #middle, #badge { display: inline-block; }
                #badge { min-width: 48px; height: 16px; }
            </style>
            <div>
                <span id="outer"><span id="middle"><span id="badge">x</span></span></span><span id="following">definition</span>
            </div>)";
        auto document = litehtml::document::createFromString(html, &container);
        if(!document)
        {
            return false;
        }
        document->render(320, litehtml::render_all);

        auto outer     = document->root()->select_one("#outer");
        auto middle    = document->root()->select_one("#middle");
        auto badge     = document->root()->select_one("#badge");
        auto following = document->root()->select_one("#following");
        if(!outer || !middle || !badge || !following || following->children().empty())
        {
            return false;
        }

        const auto outer_box     = outer->get_placement();
        const auto middle_box    = middle->get_placement();
        const auto badge_box     = badge->get_placement();
        const auto following_box = following->children().front()->get_placement();
        const float expected_width = 48.0f;
        const bool widths_preserved = outer_box.width.value() >= expected_width &&
                                      middle_box.width.value() >= expected_width &&
                                      badge_box.width.value() >= expected_width;
        const bool following_text_is_not_overlapped =
            following_box.x.value() >= outer_box.x.value() + outer_box.width.value();
        if(!widths_preserved || !following_text_is_not_overlapped)
        {
            std::cerr << "nested inline-block width propagation failed: outer=" << outer_box.width.value()
                      << " middle=" << middle_box.width.value() << " badge=" << badge_box.width.value()
                      << " following_x=" << following_box.x.value() << '\n';
        }
        return widths_preserved && following_text_is_not_overlapped;
    }

    bool inline_block_honors_explicit_width_during_intrinsic_measurement()
    {
        metrics_test_container container;
        const char* html = R"(
            <style>
                #control { display: inline-block; width: 32px; }
            </style>
            <div>
                <span id="control">wide content</span><span id="following">definition</span>
            </div>)";
        auto document = litehtml::document::createFromString(html, &container);
        if(!document)
        {
            return false;
        }
        document->render(320, litehtml::render_all);

        auto control   = document->root()->select_one("#control");
        auto following = document->root()->select_one("#following");
        if(!control || !following || following->children().empty())
        {
            return false;
        }

        const auto control_box   = control->get_placement();
        const auto following_box = following->children().front()->get_placement();
        const bool width_resolved = control_box.width.value() == 32.0f;
        const bool following_text_is_not_overlapped =
            following_box.x.value() >= control_box.x.value() + control_box.width.value();
        if(!width_resolved || !following_text_is_not_overlapped)
        {
            std::cerr << "explicit inline-block width propagation failed: control=" << control_box.width.value()
                      << " following_x=" << following_box.x.value() << '\n';
        }
        return width_resolved && following_text_is_not_overlapped;
    }

    bool nested_inline_block_inside_an_inline_container_reserves_space()
    {
        metrics_test_container container;
        const char* html = R"(
            <style>
                #middle, #badge { display: inline-block; }
                #badge { min-width: 48px; height: 16px; }
            </style>
            <div>
                <span id="wrapper"><div id="middle"><span id="badge">x</span></div></span><span id="following">definition</span>
            </div>)";
        auto document = litehtml::document::createFromString(html, &container);
        if(!document)
        {
            return false;
        }
        document->render(320, litehtml::render_all);

        auto middle    = document->root()->select_one("#middle");
        auto badge     = document->root()->select_one("#badge");
        auto following = document->root()->select_one("#following");
        if(!middle || !badge || !following || following->children().empty())
        {
            return false;
        }

        const auto middle_box    = middle->get_placement();
        const auto badge_box     = badge->get_placement();
        const auto following_box = following->children().front()->get_placement();
        const bool widths_preserved = middle_box.width.value() >= 48.0f && badge_box.width.value() >= 48.0f;
        const bool following_text_is_not_overlapped =
            following_box.x.value() >= middle_box.x.value() + middle_box.width.value();
        if(!widths_preserved || !following_text_is_not_overlapped)
        {
            std::cerr << "nested inline container width propagation failed: middle=" << middle_box.width.value()
                      << " badge=" << badge_box.width.value() << " following_x=" << following_box.x.value() << '\n';
        }
        return widths_preserved && following_text_is_not_overlapped;
    }

    bool inline_block_intrinsic_measurement_preserves_explicit_height_and_box_model()
    {
        metrics_test_container container;
        const char* html = R"(
            <style>
                #control {
                    display: inline-block;
                    width: 32px;
                    height: 20px;
                    margin-right: 7px;
                    padding: 4px 6px;
                    border: 2px solid black;
                    background: black;
                }
            </style>
            <div>
                <span id="control"></span><span id="following">definition</span>
            </div>)";
        auto document = litehtml::document::createFromString(html, &container);
        if(!document)
        {
            return false;
        }
        document->render(320, litehtml::render_all);

        auto control   = document->root()->select_one("#control");
        auto following = document->root()->select_one("#following");
        if(!control || !following || following->children().empty())
        {
            return false;
        }

        const auto control_box   = control->get_placement();
        const auto following_box = following->children().front()->get_placement();
        const bool dimensions_preserved = control_box.width.value() >= 31.5f && control_box.width.value() <= 32.5f &&
                                          control_box.height.value() >= 19.5f && control_box.height.value() <= 20.5f;
        // Placement is the content box, so only the right-side padding and
        // border separate its right edge from the following inline content.
        const float minimum_following_x = control_box.x.value() + 32.0f + 6.0f + 2.0f + 7.0f;
        const bool box_model_preserved = following_box.x.value() >= minimum_following_x - 0.5f;
        if(!dimensions_preserved || !box_model_preserved)
        {
            std::cerr << "inline-block explicit dimensions during intrinsic measurement failed: width="
                      << control_box.width.value() << " height=" << control_box.height.value()
                      << " control_x=" << control_box.x.value()
                      << " following_x=" << following_box.x.value()
                      << " expected_following_x=" << minimum_following_x << '\n';
        }
        return dimensions_preserved && box_model_preserved;
    }
} // namespace

int main()
{
    return nested_inline_block_reserves_its_resolved_width() &&
                   inline_block_honors_explicit_width_during_intrinsic_measurement() &&
                   nested_inline_block_inside_an_inline_container_reserves_space() &&
                   inline_block_intrinsic_measurement_preserves_explicit_height_and_box_model()
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
