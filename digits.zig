const png_data = @embedFile("./digits.png");
pub export const png_data_ptr = &png_data[0];
pub export const png_data_len = png_data.len;
