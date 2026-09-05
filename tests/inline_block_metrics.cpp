#include <litehtml.h>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

namespace
{
    class metrics_test_container final : public litehtml::document_container
    {
      public:
        struct drawn_text
        {
            std::string        text;
            litehtml::position pos;
        };

        std::vector<drawn_text> drawn_texts;

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

        void draw_text(litehtml::uint_ptr, const char* text, litehtml::uint_ptr, litehtml::web_color,
                       const litehtml::position& pos) override
        {
            drawn_texts.push_back({text ? text : "", pos});
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

    bool full_width_inline_blocks_wrap_after_inline_dictionary_content()
    {
        metrics_test_container container;
        const char* html = R"(
            <style>
                .entry { margin-left: 22px; }
                .collapse {
                    display: inline-block;
                    width: 100%;
                    margin: 6px 0;
                    border-left: 3px solid black;
                }
            </style>
            <div class="entry">
                <span id="prefix">dictionary note</span>
                <div class="collapse" id="first">First disclosure</div>
                <div class="collapse" id="second">Second disclosure</div>
            </div>)";
        auto document = litehtml::document::createFromString(html, &container);
        if(!document)
        {
            return false;
        }
        document->render(320, litehtml::render_all);

        auto entry  = document->root()->select_one(".entry");
        auto prefix = document->root()->select_one("#prefix");
        auto first  = document->root()->select_one("#first");
        auto second = document->root()->select_one("#second");
        if(!entry || !prefix || !first || !second)
        {
            return false;
        }

        const auto entry_box  = entry->get_placement();
        const auto prefix_box = prefix->get_placement();
        const auto first_box  = first->get_placement();
        const auto second_box = second->get_placement();
        const bool uses_entry_width = first_box.width.value() >= entry_box.width.value() - 0.5f &&
                                      second_box.width.value() >= entry_box.width.value() - 0.5f;
        const bool first_starts_on_a_new_line = first_box.y.value() >=
                                                 prefix_box.y.value() + prefix_box.height.value() - 0.5f;
        const bool second_starts_on_a_new_line = std::abs(second_box.x.value() - first_box.x.value()) <= 0.5f &&
                                                 second_box.y.value() > first_box.y.value();
        if(!uses_entry_width || !first_starts_on_a_new_line || !second_starts_on_a_new_line)
        {
            std::cerr << "full-width inline-block line placement failed: entry_width=" << entry_box.width.value()
                      << " first=(" << first_box.x.value() << ", " << first_box.y.value() << ", "
                      << first_box.width.value() << ") second=(" << second_box.x.value() << ", "
                      << second_box.y.value() << ", " << second_box.width.value() << ")\n";
        }
        return uses_entry_width && first_starts_on_a_new_line && second_starts_on_a_new_line;
    }

    bool length_vertical_align_shifts_inline_boxes_and_resolves_percentages()
    {
        metrics_test_container container;
        const char* html = R"(
            <style>
                .control { display: inline-block; width: 24px; height: 28px; }
                #lowered { vertical-align: -6px; }
                #raised { line-height: 24px; vertical-align: 25%; }
            </style>
            <div><span id="reference" class="control"></span><span id="lowered" class="control"></span><span id="raised" class="control"></span><span>phonetic</span></div>)";
        auto document = litehtml::document::createFromString(html, &container);
        if(!document)
        {
            return false;
        }
        document->render(320, litehtml::render_all);

        auto reference = document->root()->select_one("#reference");
        auto lowered   = document->root()->select_one("#lowered");
        auto raised    = document->root()->select_one("#raised");
        if(!reference || !lowered || !raised)
        {
            return false;
        }

        const auto reference_box = reference->get_placement();
        const auto lowered_box   = lowered->get_placement();
        const bool length_parsed = std::abs((lowered->css().get_vertical_align_offset() - litehtml::pixel_t(-6)).value()) <= 0.5f;
        const bool percentage_resolved = std::abs((raised->css().get_vertical_align_offset() - litehtml::pixel_t(6)).value()) <= 0.5f;
        const bool lowered_box_shifted =
            std::abs((lowered_box.y - reference_box.y - litehtml::pixel_t(6)).value()) <= 0.5f;
        if(!length_parsed || !percentage_resolved || !lowered_box_shifted)
        {
            std::cerr << "length vertical-align failed: offset=" << lowered->css().get_vertical_align_offset().value()
                      << " percent=" << raised->css().get_vertical_align_offset().value()
                      << " shift=" << (lowered_box.y - reference_box.y).value() << '\n';
        }
        return length_parsed && percentage_resolved && lowered_box_shifted;
    }

    bool absolute_generated_content_uses_inline_static_baseline()
    {
        metrics_test_container container;
        const char* html = R"(
            <style>
                .entry { margin-left: 22px; }
                ol { counter-reset: item; list-style: none; margin: 0; padding: 0; }
                li::before {
                    content: counter(item) " ";
                    counter-increment: item;
                    font-size: 14px;
                    margin-left: -1.3rem;
                    position: absolute;
                    text-align: right;
                }
                .badge { display: inline-block; width: 45px; height: 23px; vertical-align: middle; }
            </style>
            <div class="entry"><ol><li><span class="badge">A1</span> <span>definition</span></li></ol></div>)";
        auto document = litehtml::document::createFromString(html, &container);
        if(!document)
        {
            return false;
        }
        document->render(320, litehtml::render_all);
        document->draw(0, 0, 0, nullptr);

        const metrics_test_container::drawn_text* marker = nullptr;
        const metrics_test_container::drawn_text* definition = nullptr;
        for(const auto& drawn : container.drawn_texts)
        {
            if(drawn.text == "1") marker = &drawn;
            if(drawn.text == "definition") definition = &drawn;
        }
        if(!marker || !definition)
        {
            return false;
        }

        const auto marker_baseline = marker->pos.y + litehtml::pixel_t(14) * 3 / 4;
        const auto definition_baseline = definition->pos.y + litehtml::pixel_t(16) * 3 / 4;
        const bool baselines_aligned =
            std::abs((marker_baseline - definition_baseline).value()) <= 0.5f;
        if(!baselines_aligned)
        {
            std::cerr << "absolute generated content static baseline failed: marker=" << marker_baseline.value()
                      << " definition=" << definition_baseline.value() << '\n';
        }
        return baselines_aligned;
    }

    bool absolute_inline_content_uses_static_baseline()
    {
        metrics_test_container container;
        const char* html = R"(
            <style>
                #marker { position: absolute; font-size: 14px; margin-left: -1.3rem; }
                #badge { display: inline-block; width: 45px; height: 23px; vertical-align: middle; }
            </style>
            <div><span id="marker">1</span><span id="badge">A1</span> <span>definition</span></div>)";
        auto document = litehtml::document::createFromString(html, &container);
        if(!document)
        {
            return false;
        }
        document->render(320, litehtml::render_all);
        document->draw(0, 0, 0, nullptr);

        const metrics_test_container::drawn_text* marker = nullptr;
        const metrics_test_container::drawn_text* definition = nullptr;
        for(const auto& drawn : container.drawn_texts)
        {
            if(drawn.text == "1") marker = &drawn;
            if(drawn.text == "definition") definition = &drawn;
        }
        if(!marker || !definition)
        {
            return false;
        }

        const auto marker_baseline = marker->pos.y + litehtml::pixel_t(14) * 3 / 4;
        const auto definition_baseline = definition->pos.y + litehtml::pixel_t(16) * 3 / 4;
        const bool baselines_aligned =
            std::abs((marker_baseline - definition_baseline).value()) <= 0.5f;
        if(!baselines_aligned)
        {
            std::cerr << "absolute inline static baseline failed: marker=" << marker_baseline.value()
                      << " definition=" << definition_baseline.value() << '\n';
        }
        return baselines_aligned;
    }
} // namespace

int main()
{
    return nested_inline_block_reserves_its_resolved_width() &&
                   inline_block_honors_explicit_width_during_intrinsic_measurement() &&
                   nested_inline_block_inside_an_inline_container_reserves_space() &&
                   inline_block_intrinsic_measurement_preserves_explicit_height_and_box_model() &&
                   full_width_inline_blocks_wrap_after_inline_dictionary_content() &&
                   length_vertical_align_shifts_inline_boxes_and_resolves_percentages() &&
                   absolute_generated_content_uses_inline_static_baseline() &&
                   absolute_inline_content_uses_static_baseline()
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
