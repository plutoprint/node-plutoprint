export type SizeType =
    | 'a3'
    | 'a4'
    | 'a5'
    | 'b4'
    | 'b5'
    | 'letter'
    | 'legal'
    | 'ledger';

export type MediaType = 'print' | 'screen';

export type LengthType = number | string;

export interface BookOptions {
    size?: SizeType;
    media?: MediaType;
    width?: LengthType;
    height?: LengthType;
    margin?: LengthType;
    marginTop?: LengthType;
    marginRight?: LengthType;
    marginBottom?: LengthType;
    marginLeft?: LengthType;
    title?: string;
    subject?: string;
    author?: string;
    keywords?: string;
    creator?: string;
    creationDate?: Date;
    modificationDate?: Date;
}

export interface LoadOptions {
    userStyle?: string;
    userScript?: string;
}

export interface LoadContentOptions extends LoadOptions {
    baseUrl?: string;
}

export interface LoadDataOptions extends LoadContentOptions {
    mimeType?: string;
    textEncoding?: string;
}

export interface WritePdfOptions {
    pageStart?: number;
    pageEnd?: number;
    pageStep?: number;
}

export interface WritePngOptions {
    width?: number;
    height?: number;
}

export class Book {
    constructor(options?: BookOptions);

    readonly pageCount: number;
    readonly documentWidth: number;
    readonly documentHeight: number;
    readonly viewportWidth: number;
    readonly viewportHeight: number;

    loadUrl(url: string, options?: LoadOptions): this;
    loadHtml(content: string, options?: LoadContentOptions): this;
    loadXml(content: string, options?: LoadContentOptions): this;
    loadData(buffer: Buffer, options?: LoadDataOptions): this;
    loadImage(buffer: Buffer, options?: LoadDataOptions): this;

    writeToPdf(path: string, options?: WritePdfOptions): void;
    writeToPdfBuffer(options?: WritePdfOptions): Buffer;

    writeToPng(path: string, options?: WritePngOptions): void;
    writeToPngBuffer(options?: WritePngOptions): Buffer;
}

export function createBook(options?: BookOptions): Book;

export const plutobookVersion: string;
export const plutobookBuildInfo: string;

export const MIN_PAGE_COUNT: number;
export const MAX_PAGE_COUNT: number;

export const UNITS_PT: number;
export const UNITS_PC: number;
export const UNITS_IN: number;
export const UNITS_CM: number;
export const UNITS_MM: number;
export const UNITS_PX: number;
